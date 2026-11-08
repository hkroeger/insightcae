#!/usr/bin/env bash
#
# create-vm-image.sh
#
# Builds an AWS EC2 machine image (AMI) for later instantiation of VMs:
#   1. launches a small builder instance (with temporary key pair and security group)
#   2. uploads and executes the setup script embedded below (see setup_script())
#   3. cleans up instance-specific state and stops the builder
#   4. creates an AMI from the stopped builder
#   5. terminates the builder and deletes the temporary key pair and security group
#
# Requirements: AWS CLI v2 ("aws"), configured credentials, ssh, scp, ssh-keygen.
#
# All settings below may also be overridden from the environment, e.g.
#   IMAGE_NAME=insightcae-test REGION=eu-west-1 ./create-vm-image.sh
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- AWS account / location
AWS_PROFILE="${AWS_PROFILE:-}"                   # empty: default credentials
REGION="${REGION:-eu-central-1}"

# --- Resulting image
IMAGE_NAME="${IMAGE_NAME:-insightcae-ubuntu2404-$(date +%Y%m%d-%H%M)}"
IMAGE_DESCRIPTION="${IMAGE_DESCRIPTION:-InsightCAE on Ubuntu 24.04}"
IMAGE_TAGS="${IMAGE_TAGS:-purpose=insightcae created-by=create-vm-image.sh}"

# --- Builder instance (minimal resources, only used during the build)
BUILDER_NAME="${BUILDER_NAME:-${IMAGE_NAME}-builder}"
BUILDER_INSTANCE_TYPE="${BUILDER_INSTANCE_TYPE:-t3.small}"
BUILDER_ROOT_VOLUME_SIZE_GB="${BUILDER_ROOT_VOLUME_SIZE_GB:-30}"
BUILDER_ROOT_VOLUME_TYPE="${BUILDER_ROOT_VOLUME_TYPE:-gp3}"
# Base AMI: explicit ID, or the newest AMI matching owner and name pattern
BASE_AMI="${BASE_AMI:-}"
BASE_AMI_OWNER="${BASE_AMI_OWNER:-099720109477}"  # Canonical
BASE_AMI_NAME_PATTERN="${BASE_AMI_NAME_PATTERN:-ubuntu/images/hvm-ssd-gp3/ubuntu-noble-24.04-amd64-server-*}"
# Login user of the base AMI (fixed by the AMI, "ubuntu" for Ubuntu)
BUILDER_SSH_USER="${BUILDER_SSH_USER:-ubuntu}"
# SSH key used to log into the builder; generated temporarily if empty
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"
# Source address allowed to reach the builder via SSH (CIDR)
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-0.0.0.0/0}"
# Network: empty values select the default VPC and its default subnet
VPC_ID="${VPC_ID:-}"
SUBNET_ID="${SUBNET_ID:-}"

# --- Contents of the image (consumed by the embedded setup script)
SD_REPO_URL="${SD_REPO_URL:-http://downloads.silentdynamics.de/ubuntu}"
SD_REPO_KEY_URL="${SD_REPO_KEY_URL:-http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg}"
INSTALL_PACKAGES="${INSTALL_PACKAGES:-insightcae}"
EXTRA_PACKAGES="${EXTRA_PACKAGES:-htop tmux rsync}"
INSIGHT_SETENV="${INSIGHT_SETENV:-/opt/insightcae/bin/insight_setenv.sh}"

# --- Behaviour
KEEP_BUILDER_ON_FAILURE="${KEEP_BUILDER_ON_FAILURE:-no}"   # "yes": keep builder for debugging if something fails
IMAGE_WAIT_MINUTES="${IMAGE_WAIT_MINUTES:-60}"             # max. time to wait for the AMI to become available


###############################################################################
# Setup script executed inside the builder instance (as root)
###############################################################################
setup_script() {
cat <<'SETUP_EOF'
#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

echo "=== waiting for cloud-init to finish"
cloud-init status --wait >/dev/null 2>&1 || true

echo "=== updating base system"
apt-get update
apt-get -y -o Dpkg::Options::=--force-confold dist-upgrade
apt-get -y install ca-certificates curl gnupg lsb-release

echo "=== adding silentdynamics apt repository"
install -d -m 0755 /etc/apt/keyrings
curl -fsSL "$SD_REPO_KEY_URL" -o /tmp/sd-key
if grep -q "BEGIN PGP" /tmp/sd-key; then
  gpg --dearmor --yes -o /etc/apt/keyrings/silentdynamics.gpg /tmp/sd-key
else
  install -m 0644 /tmp/sd-key /etc/apt/keyrings/silentdynamics.gpg
fi
rm -f /tmp/sd-key
echo "deb [signed-by=/etc/apt/keyrings/silentdynamics.gpg] $SD_REPO_URL $(lsb_release -cs) main" \
  > /etc/apt/sources.list.d/silentdynamics.list
apt-get update

echo "=== installing packages: $INSTALL_PACKAGES $EXTRA_PACKAGES"
# shellcheck disable=SC2086
apt-get -y install $INSTALL_PACKAGES $EXTRA_PACKAGES

echo "=== setting up InsightCAE environment for all future users"
# The environment is configured via /etc/skel, from which the users of
# instantiated VMs are created.
if ! grep -q insight_setenv /etc/skel/.bashrc; then
  { echo "source $INSIGHT_SETENV"; cat /etc/skel/.bashrc; } > /tmp/skel.bashrc
  install -m 0644 /tmp/skel.bashrc /etc/skel/.bashrc
  rm -f /tmp/skel.bashrc
fi

echo "=== cleaning up"
apt-get -y autoremove --purge
apt-get clean
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
journalctl --rotate >/dev/null 2>&1 || true
journalctl --vacuum-time=1s >/dev/null 2>&1 || true

echo "=== setup finished"
SETUP_EOF
}


###############################################################################
# Implementation
###############################################################################

log() { echo -e "\n[$(date +%H:%M:%S)] $*"; }
ec2() { aws ec2 --region "$REGION" --output text "$@"; }
[[ -n "$AWS_PROFILE" ]] && export AWS_PROFILE || unset AWS_PROFILE

# AWS CLI style tag list from "k=v k=v"
tag_list() {
  local out="" kv
  for kv in $IMAGE_TAGS "$@"; do out+="{Key=${kv%%=*},Value=${kv#*=}},"; done
  echo "${out%,}"
}

WORKDIR="$(mktemp -d)"
BUILD_OK=no
INSTANCE_ID=""
SG_ID=""
KEY_NAME=""

cleanup() {
  local rc=$?
  if [[ "$BUILD_OK" != yes && "$KEEP_BUILDER_ON_FAILURE" == yes && -n "$INSTANCE_ID" ]]; then
    log "Build failed; keeping builder instance $INSTANCE_ID, security group $SG_ID and key pair $KEY_NAME."
    [[ -n "${BUILDER_IP:-}" ]] && echo "  ssh -i $SSH_PRIVATE_KEY_FILE $BUILDER_SSH_USER@$BUILDER_IP"
    exit $rc
  fi
  if [[ -n "$INSTANCE_ID" ]]; then
    log "Terminating builder instance $INSTANCE_ID"
    ec2 terminate-instances --instance-ids "$INSTANCE_ID" >/dev/null || true
    ec2 wait instance-terminated --instance-ids "$INSTANCE_ID" || true
  fi
  if [[ -n "$SG_ID" ]]; then
    log "Deleting security group $SG_ID"
    ec2 delete-security-group --group-id "$SG_ID" || true
  fi
  if [[ -n "$KEY_NAME" ]]; then
    ec2 delete-key-pair --key-name "$KEY_NAME" || true
  fi
  rm -rf "$WORKDIR"
  exit $rc
}
trap cleanup EXIT

for cmd in aws ssh scp ssh-keygen; do
  command -v "$cmd" >/dev/null || { echo "Required command '$cmd' not found." >&2; exit 1; }
done
aws sts get-caller-identity >/dev/null 2>&1 || { echo "No valid AWS credentials. Run 'aws configure' or 'aws sso login' first." >&2; exit 1; }

if [[ -n "$(ec2 describe-images --owners self --filters "Name=name,Values=$IMAGE_NAME" --query 'Images[].ImageId')" ]]; then
  echo "An AMI named '$IMAGE_NAME' already exists." >&2
  exit 1
fi

# --- SSH key
if [[ -z "$SSH_PUBLIC_KEY_FILE" ]]; then
  ssh-keygen -q -t ed25519 -N "" -C "image-builder" -f "$WORKDIR/id_builder"
  SSH_PUBLIC_KEY_FILE="$WORKDIR/id_builder.pub"
  SSH_PRIVATE_KEY_FILE="$WORKDIR/id_builder"
else
  SSH_PRIVATE_KEY_FILE="${SSH_PUBLIC_KEY_FILE%.pub}"
fi
SSH_OPTS=(-i "$SSH_PRIVATE_KEY_FILE" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
          -o ConnectTimeout=10 -o ServerAliveInterval=30 -o LogLevel=ERROR)

log "Account: $(aws sts get-caller-identity --query Account --output text), region $REGION"

# --- base image
if [[ -z "$BASE_AMI" ]]; then
  BASE_AMI=$(ec2 describe-images --owners "$BASE_AMI_OWNER" \
    --filters "Name=name,Values=$BASE_AMI_NAME_PATTERN" "Name=state,Values=available" \
    --query 'sort_by(Images, &CreationDate)[-1].ImageId')
  [[ -n "$BASE_AMI" && "$BASE_AMI" != None ]] || { echo "No base AMI found." >&2; exit 1; }
fi
ROOT_DEVICE=$(ec2 describe-images --image-ids "$BASE_AMI" --query 'Images[0].RootDeviceName')
log "Building AMI '$IMAGE_NAME' from base AMI $BASE_AMI"

# --- network
if [[ -z "$VPC_ID" ]]; then
  if [[ -n "$SUBNET_ID" ]]; then
    VPC_ID=$(ec2 describe-subnets --subnet-ids "$SUBNET_ID" --query 'Subnets[0].VpcId')
  else
    VPC_ID=$(ec2 describe-vpcs --filters Name=is-default,Values=true --query 'Vpcs[0].VpcId')
    [[ -n "$VPC_ID" && "$VPC_ID" != None ]] || { echo "No default VPC; set VPC_ID and SUBNET_ID." >&2; exit 1; }
  fi
fi

# --- temporary key pair and security group
KEY_NAME="$BUILDER_NAME"
ec2 import-key-pair --key-name "$KEY_NAME" --public-key-material "fileb://$SSH_PUBLIC_KEY_FILE" >/dev/null
SG_ID=$(ec2 create-security-group --group-name "$BUILDER_NAME" --vpc-id "$VPC_ID" \
  --description "temporary: SSH access to image builder" --query GroupId)
ec2 authorize-security-group-ingress --group-id "$SG_ID" \
  --protocol tcp --port 22 --cidr "$SSH_SOURCE_ADDRESS" >/dev/null

# --- builder instance
log "Launching builder instance '$BUILDER_NAME' ($BUILDER_INSTANCE_TYPE)"
NET_ARGS=()
[[ -n "$SUBNET_ID" ]] && NET_ARGS=(--subnet-id "$SUBNET_ID" --associate-public-ip-address)
INSTANCE_ID=$(ec2 run-instances \
  --image-id "$BASE_AMI" \
  --instance-type "$BUILDER_INSTANCE_TYPE" \
  --key-name "$KEY_NAME" \
  --security-group-ids "$SG_ID" \
  "${NET_ARGS[@]}" \
  --block-device-mappings "DeviceName=$ROOT_DEVICE,Ebs={VolumeSize=$BUILDER_ROOT_VOLUME_SIZE_GB,VolumeType=$BUILDER_ROOT_VOLUME_TYPE,DeleteOnTermination=true}" \
  --tag-specifications "ResourceType=instance,Tags=[$(tag_list "Name=$BUILDER_NAME" temporary=true)]" \
  --query 'Instances[0].InstanceId')
ec2 wait instance-running --instance-ids "$INSTANCE_ID"
BUILDER_IP=$(ec2 describe-instances --instance-ids "$INSTANCE_ID" \
  --query 'Reservations[0].Instances[0].PublicIpAddress')
[[ -n "$BUILDER_IP" && "$BUILDER_IP" != None ]] || { echo "Builder has no public IP (check subnet settings)." >&2; exit 1; }
log "Builder instance $INSTANCE_ID is at $BUILDER_IP"

log "Waiting for SSH"
for i in $(seq 1 60); do
  ssh "${SSH_OPTS[@]}" "$BUILDER_SSH_USER@$BUILDER_IP" true 2>/dev/null && break
  [[ $i -eq 60 ]] && { echo "SSH did not come up." >&2; exit 1; }
  sleep 5
done

# --- run setup
log "Uploading and running setup script"
setup_script > "$WORKDIR/setup.sh"
scp "${SSH_OPTS[@]}" "$WORKDIR/setup.sh" "$BUILDER_SSH_USER@$BUILDER_IP:/tmp/image-setup.sh"
REMOTE_ENV=""
for v in SD_REPO_URL SD_REPO_KEY_URL INSTALL_PACKAGES EXTRA_PACKAGES INSIGHT_SETENV; do
  REMOTE_ENV+=" $v=$(printf %q "${!v}")"
done
ssh "${SSH_OPTS[@]}" "$BUILDER_SSH_USER@$BUILDER_IP" "sudo env$REMOTE_ENV bash /tmp/image-setup.sh"

# --- remove instance-specific state, so that cloud-init provisions new instances from scratch
log "Cleaning instance-specific state"
ssh "${SSH_OPTS[@]}" "$BUILDER_SSH_USER@$BUILDER_IP" \
  "sudo cloud-init clean --logs --machine-id; sudo rm -f /etc/ssh/ssh_host_*; \
   sudo rm -f /root/.bash_history ~/.bash_history; rm -f ~/.ssh/authorized_keys" || true

log "Stopping builder instance"
ec2 stop-instances --instance-ids "$INSTANCE_ID" >/dev/null
ec2 wait instance-stopped --instance-ids "$INSTANCE_ID"

# --- create AMI
log "Creating AMI '$IMAGE_NAME'"
IMAGE_ID=$(ec2 create-image --instance-id "$INSTANCE_ID" --no-reboot \
  --name "$IMAGE_NAME" --description "$IMAGE_DESCRIPTION" \
  --tag-specifications "ResourceType=image,Tags=[$(tag_list "Name=$IMAGE_NAME")]" \
                       "ResourceType=snapshot,Tags=[$(tag_list "Name=$IMAGE_NAME")]" \
  --query ImageId)
log "Waiting for AMI $IMAGE_ID to become available"
for i in $(seq 1 $((IMAGE_WAIT_MINUTES * 4))); do
  state=$(ec2 describe-images --image-ids "$IMAGE_ID" --query 'Images[0].State')
  [[ "$state" == available ]] && break
  [[ "$state" == failed ]] && { echo "AMI creation failed." >&2; exit 1; }
  [[ $i -eq $((IMAGE_WAIT_MINUTES * 4)) ]] && { echo "Timeout waiting for AMI." >&2; exit 1; }
  sleep 15
done

BUILD_OK=yes

log "Done. AMI ID:"
echo "  $IMAGE_ID ($REGION)"
cat <<EOF

Create a VM from it with e.g.:
  ./aws-vm.sh --create <vm-name> --image $IMAGE_ID
EOF
