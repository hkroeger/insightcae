#!/usr/bin/env bash
#
# aws-vm.sh
#
# Creates or removes an AWS EC2 instance based on an AMI built by create-vm-image.sh.
#
# Usage:
#   aws-vm.sh -c|--create <vm-name> [options]
#   aws-vm.sh -r|--remove <vm-name> [options]
#
# Options:
#   -s, --size <instance-type>  instance type
#   -u, --user <name>           admin user name
#   -k, --ssh-key <file.pub>    public SSH key
#   -i, --image <ami-id>        AMI ID
#   -y, --yes                   do not ask for confirmation on removal
#   -h, --help                  show this help
#
# Each instance gets its own security group (<vm-name>-sg) and, optionally,
# an Elastic IP. All resources are tagged with Name=<vm-name> and
# managed-by=aws-vm.sh; removal deletes exactly these resources.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- AWS account / location
AWS_PROFILE="${AWS_PROFILE:-}"                   # empty: default credentials
REGION="${REGION:-eu-central-1}"

# --- Source image: explicit AMI ID, or the newest own AMI whose name starts with IMAGE_NAME_PREFIX
IMAGE_ID="${IMAGE_ID:-}"
IMAGE_NAME_PREFIX="${IMAGE_NAME_PREFIX:-insightcae-ubuntu2404-}"

# --- VM
INSTANCE_TYPE="${INSTANCE_TYPE:-m6i.xlarge}"
ROOT_VOLUME_SIZE_GB="${ROOT_VOLUME_SIZE_GB:-64}"
ROOT_VOLUME_TYPE="${ROOT_VOLUME_TYPE:-gp3}"
DATA_VOLUME_SIZE_GB="${DATA_VOLUME_SIZE_GB:-0}"  # 0: no data volume
DATA_VOLUME_TYPE="${DATA_VOLUME_TYPE:-gp3}"
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"   # empty: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-0.0.0.0/0}"  # CIDR allowed to reach the VM via SSH
VPC_ID="${VPC_ID:-}"                             # empty: default VPC (or VPC of SUBNET_ID)
SUBNET_ID="${SUBNET_ID:-}"                       # empty: default subnet
USE_ELASTIC_IP="${USE_ELASTIC_IP:-no}"           # "yes": static public IP (survives stop/start, billed separately)
AUTO_SHUTDOWN_TIME="${AUTO_SHUTDOWN_TIME:-}"     # e.g. "1900" (UTC) for daily auto-shutdown; empty: off
VM_TAGS="${VM_TAGS:-purpose=insightcae}"

MANAGED_TAG_KEY="managed-by"
MANAGED_TAG_VALUE="aws-vm.sh"


###############################################################################
# Implementation
###############################################################################

log() { echo -e "[$(date +%H:%M:%S)] $*"; }
die() { echo "Error: $*" >&2; exit 1; }
ec2() { aws ec2 --region "$REGION" --output text "$@"; }
[[ -n "$AWS_PROFILE" ]] && export AWS_PROFILE || unset AWS_PROFILE

usage() {
  cat <<EOF
Usage:
  $(basename "$0") -c|--create <vm-name> [options]
  $(basename "$0") -r|--remove <vm-name> [options]

Options:
  -s, --size <instance-type>  instance type (default: $INSTANCE_TYPE)
  -u, --user <name>           admin user name (default: $ADMIN_USERNAME)
  -k, --ssh-key <file.pub>    public SSH key (default: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub)
  -i, --image <ami-id>        AMI ID (default: newest own AMI named '${IMAGE_NAME_PREFIX}*')
  -y, --yes                   do not ask for confirmation on removal
  -h, --help                  show this help
EOF
  exit "${1:-0}"
}

MODE=""
VM_NAME=""
ASSUME_YES=no

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--create)  MODE=create; VM_NAME="${2:-}"; shift 2 || usage 1 ;;
    -r|--remove)  MODE=remove; VM_NAME="${2:-}"; shift 2 || usage 1 ;;
    -s|--size)    INSTANCE_TYPE="$2"; shift 2 ;;
    -u|--user)    ADMIN_USERNAME="$2"; shift 2 ;;
    -k|--ssh-key) SSH_PUBLIC_KEY_FILE="$2"; shift 2 ;;
    -i|--image)   IMAGE_ID="$2"; shift 2 ;;
    -y|--yes)     ASSUME_YES=yes; shift ;;
    -h|--help)    usage 0 ;;
    *)            echo "Unknown argument: $1" >&2; usage 1 ;;
  esac
done

[[ -n "$MODE" ]]    || { echo "Either --create or --remove is required." >&2; usage 1; }
[[ -n "$VM_NAME" ]] || { echo "A VM name is required." >&2; usage 1; }
[[ "$VM_NAME" =~ ^[a-zA-Z0-9][a-zA-Z0-9-]{0,62}$ ]] || die "invalid VM name '$VM_NAME' (letters, digits, '-')"

command -v aws >/dev/null || die "AWS CLI 'aws' not found."
aws sts get-caller-identity >/dev/null 2>&1 || die "no valid AWS credentials. Run 'aws configure' or 'aws sso login' first."

OWN_FILTERS=("Name=tag:Name,Values=$VM_NAME" "Name=tag:$MANAGED_TAG_KEY,Values=$MANAGED_TAG_VALUE")

# AWS CLI style tag list from "k=v k=v"
tag_list() {
  local out="" kv
  for kv in $VM_TAGS "Name=$VM_NAME" "$MANAGED_TAG_KEY=$MANAGED_TAG_VALUE"; do
    out+="{Key=${kv%%=*},Value=${kv#*=}},"
  done
  echo "${out%,}"
}

find_instance() {
  ec2 describe-instances --filters "${OWN_FILTERS[@]}" \
    "Name=instance-state-name,Values=pending,running,stopping,stopped" \
    --query 'Reservations[].Instances[].InstanceId'
}


create_vm() {
  if [[ -z "$SSH_PUBLIC_KEY_FILE" ]]; then
    for k in ~/.ssh/id_ed25519.pub ~/.ssh/id_rsa.pub; do
      [[ -f "$k" ]] && { SSH_PUBLIC_KEY_FILE="$k"; break; }
    done
  fi
  [[ -f "$SSH_PUBLIC_KEY_FILE" ]] || die "no SSH public key found; specify one with --ssh-key"
  [[ -z "$(find_instance)" ]] || die "VM '$VM_NAME' already exists"
  if [[ -n "$AUTO_SHUTDOWN_TIME" ]]; then
    [[ "$AUTO_SHUTDOWN_TIME" =~ ^([01][0-9]|2[0-3])[0-5][0-9]$ ]] || die "AUTO_SHUTDOWN_TIME must be HHMM"
  fi

  if [[ -z "$IMAGE_ID" ]]; then
    IMAGE_ID=$(ec2 describe-images --owners self \
      --filters "Name=name,Values=${IMAGE_NAME_PREFIX}*" "Name=state,Values=available" \
      --query 'sort_by(Images, &CreationDate)[-1].ImageId')
    [[ -n "$IMAGE_ID" && "$IMAGE_ID" != None ]] || die "no AMI '${IMAGE_NAME_PREFIX}*' found in $REGION"
  fi
  local root_device
  root_device=$(ec2 describe-images --image-ids "$IMAGE_ID" --query 'Images[0].RootDeviceName')

  if [[ -z "$VPC_ID" ]]; then
    if [[ -n "$SUBNET_ID" ]]; then
      VPC_ID=$(ec2 describe-subnets --subnet-ids "$SUBNET_ID" --query 'Subnets[0].VpcId')
    else
      VPC_ID=$(ec2 describe-vpcs --filters Name=is-default,Values=true --query 'Vpcs[0].VpcId')
      [[ -n "$VPC_ID" && "$VPC_ID" != None ]] || die "no default VPC; set VPC_ID and SUBNET_ID"
    fi
  fi

  log "Creating VM '$VM_NAME' ($INSTANCE_TYPE) in $REGION from $IMAGE_ID"

  # --- security group
  local sg_id
  sg_id=$(ec2 describe-security-groups --filters "${OWN_FILTERS[@]}" "Name=vpc-id,Values=$VPC_ID" \
            --query 'SecurityGroups[0].GroupId')
  if [[ -z "$sg_id" || "$sg_id" == None ]]; then
    sg_id=$(ec2 create-security-group --group-name "${VM_NAME}-sg" --vpc-id "$VPC_ID" \
      --description "SSH access to $VM_NAME" \
      --tag-specifications "ResourceType=security-group,Tags=[$(tag_list)]" --query GroupId)
    ec2 authorize-security-group-ingress --group-id "$sg_id" \
      --protocol tcp --port 22 --cidr "$SSH_SOURCE_ADDRESS" >/dev/null
  fi

  # --- user data: create admin user, optional auto-shutdown
  local user_data
  user_data="$(mktemp)"
  cat > "$user_data" <<EOF
#cloud-config
users:
  - name: $ADMIN_USERNAME
    groups: [sudo]
    sudo: "ALL=(ALL) NOPASSWD:ALL"
    shell: /bin/bash
    ssh_authorized_keys:
      - $(cat "$SSH_PUBLIC_KEY_FILE")
EOF
  if [[ -n "$AUTO_SHUTDOWN_TIME" ]]; then
    cat >> "$user_data" <<EOF
write_files:
  - path: /etc/cron.d/auto-shutdown
    permissions: "0644"
    content: |
      ${AUTO_SHUTDOWN_TIME:2:2} ${AUTO_SHUTDOWN_TIME:0:2} * * * root /sbin/shutdown -h now
EOF
  fi

  # --- block devices
  local bdm="[{\"DeviceName\":\"$root_device\",\"Ebs\":{\"VolumeSize\":$ROOT_VOLUME_SIZE_GB,\"VolumeType\":\"$ROOT_VOLUME_TYPE\",\"DeleteOnTermination\":true}}"
  if [[ "$DATA_VOLUME_SIZE_GB" -gt 0 ]]; then
    bdm+=",{\"DeviceName\":\"/dev/sdf\",\"Ebs\":{\"VolumeSize\":$DATA_VOLUME_SIZE_GB,\"VolumeType\":\"$DATA_VOLUME_TYPE\",\"DeleteOnTermination\":true}}"
  fi
  bdm+="]"

  local net_args=()
  [[ -n "$SUBNET_ID" ]] && net_args=(--subnet-id "$SUBNET_ID" --associate-public-ip-address)

  local instance_id
  instance_id=$(ec2 run-instances \
    --image-id "$IMAGE_ID" \
    --instance-type "$INSTANCE_TYPE" \
    --security-group-ids "$sg_id" \
    "${net_args[@]}" \
    --block-device-mappings "$bdm" \
    --user-data "file://$user_data" \
    --instance-initiated-shutdown-behavior stop \
    --tag-specifications "ResourceType=instance,Tags=[$(tag_list)]" \
                         "ResourceType=volume,Tags=[$(tag_list)]" \
    --query 'Instances[0].InstanceId')
  rm -f "$user_data"
  log "Waiting for instance $instance_id"
  ec2 wait instance-running --instance-ids "$instance_id"

  if [[ "$USE_ELASTIC_IP" == yes ]]; then
    local alloc_id
    alloc_id=$(ec2 allocate-address --domain vpc \
      --tag-specifications "ResourceType=elastic-ip,Tags=[$(tag_list)]" --query AllocationId)
    ec2 associate-address --instance-id "$instance_id" --allocation-id "$alloc_id" >/dev/null
  fi

  local ip
  ip=$(ec2 describe-instances --instance-ids "$instance_id" \
         --query 'Reservations[0].Instances[0].PublicIpAddress')

  log "VM '$VM_NAME' is running."
  echo "  instance:  $instance_id"
  echo "  public IP: $ip"
  echo "  login:     ssh $ADMIN_USERNAME@$ip   (available once cloud-init has created the user)"
  if [[ "$DATA_VOLUME_SIZE_GB" -gt 0 ]]; then
    echo "  note: the data volume is attached unformatted (see 'lsblk' on the VM)."
  fi
}


remove_vm() {
  local instance_id sg_ids alloc_ids
  instance_id=$(find_instance)
  sg_ids=$(ec2 describe-security-groups --filters "${OWN_FILTERS[@]}" --query 'SecurityGroups[].GroupId')
  alloc_ids=$(ec2 describe-addresses --filters "${OWN_FILTERS[@]}" --query 'Addresses[].AllocationId')
  [[ -n "$instance_id$sg_ids$alloc_ids" ]] || die "no resources of VM '$VM_NAME' found in $REGION"

  log "The following resources of VM '$VM_NAME' will be deleted:"
  for r in $instance_id; do echo "  instance:       $r (incl. its volumes)"; done
  for r in $sg_ids;      do echo "  security group: $r"; done
  for r in $alloc_ids;   do echo "  elastic IP:     $r"; done

  if [[ "$ASSUME_YES" != yes ]]; then
    read -r -p "Permanently delete these resources? [y/N] " answer
    [[ "$answer" =~ ^[yY]([eE][sS])?$ ]] || { echo "Aborted."; exit 1; }
  fi

  if [[ -n "$instance_id" ]]; then
    log "Terminating instance(s) $instance_id"
    # shellcheck disable=SC2086
    ec2 terminate-instances --instance-ids $instance_id >/dev/null
    # shellcheck disable=SC2086
    ec2 wait instance-terminated --instance-ids $instance_id
  fi
  for r in $alloc_ids; do
    log "Releasing elastic IP $r"
    ec2 release-address --allocation-id "$r"
  done
  for r in $sg_ids; do
    log "Deleting security group $r"
    ec2 delete-security-group --group-id "$r"
  done
  log "VM '$VM_NAME' removed."
}


case "$MODE" in
  create) create_vm ;;
  remove) remove_vm ;;
esac
