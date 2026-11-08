#!/usr/bin/env bash
#
# create-vm-image.sh
#
# Builds a Google Compute Engine image for later instantiation of VMs:
#   1. creates a small builder instance and a temporary SSH firewall rule
#   2. uploads and executes the setup script embedded below (see setup_script())
#   3. removes the builder user and instance-specific state and powers off the builder
#   4. creates an image (in an image family) from the builder's boot disk
#   5. deletes the builder instance and the firewall rule
#
# Requirements: Google Cloud CLI ("gcloud"), logged in ("gcloud auth login"),
# ssh, scp, ssh-keygen.
#
# All settings below may also be overridden from the environment, e.g.
#   IMAGE_NAME=insightcae-test ZONE=europe-west4-a ./create-vm-image.sh
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- GCP project / location
PROJECT="${PROJECT:-}"                           # empty: current gcloud default project
ZONE="${ZONE:-europe-west3-c}"

# --- Resulting image
IMAGE_NAME="${IMAGE_NAME:-insightcae-ubuntu2404-$(date +%Y%m%d-%H%M)}"
IMAGE_FAMILY="${IMAGE_FAMILY:-insightcae-ubuntu2404}"
IMAGE_DESCRIPTION="${IMAGE_DESCRIPTION:-InsightCAE on Ubuntu 24.04}"
IMAGE_STORAGE_LOCATION="${IMAGE_STORAGE_LOCATION:-eu}"   # multi-region or region where the image is stored
IMAGE_LABELS="${IMAGE_LABELS:-purpose=insightcae,created-by=create-vm-image}"

# --- Builder instance (minimal resources, only used during the build)
BUILDER_NAME="${BUILDER_NAME:-${IMAGE_NAME}-builder}"
BUILDER_MACHINE_TYPE="${BUILDER_MACHINE_TYPE:-e2-small}"
BUILDER_DISK_SIZE_GB="${BUILDER_DISK_SIZE_GB:-30}"
BUILDER_DISK_TYPE="${BUILDER_DISK_TYPE:-pd-balanced}"
BASE_IMAGE_FAMILY="${BASE_IMAGE_FAMILY:-ubuntu-2404-lts-amd64}"
BASE_IMAGE_PROJECT="${BASE_IMAGE_PROJECT:-ubuntu-os-cloud}"
NETWORK="${NETWORK:-default}"
SUBNET="${SUBNET:-}"                             # empty: auto-mode subnet of NETWORK
# Temporary user on the builder (removed before the image is created)
BUILDER_USERNAME="${BUILDER_USERNAME:-imagebuilder}"
# SSH key used to log into the builder; generated temporarily if empty
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"
# Source address allowed to reach the builder via SSH (CIDR)
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-0.0.0.0/0}"

# --- Contents of the image (consumed by the embedded setup script)
SD_REPO_URL="${SD_REPO_URL:-http://downloads.silentdynamics.de/ubuntu}"
SD_REPO_KEY_URL="${SD_REPO_KEY_URL:-http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg}"
INSTALL_PACKAGES="${INSTALL_PACKAGES:-insightcae}"
EXTRA_PACKAGES="${EXTRA_PACKAGES:-htop tmux rsync}"
INSIGHT_SETENV="${INSIGHT_SETENV:-/opt/insightcae/bin/insight_setenv.sh}"

# --- Behaviour
KEEP_BUILDER_ON_FAILURE="${KEEP_BUILDER_ON_FAILURE:-no}"   # "yes": keep builder for debugging if something fails


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
# The builder user is removed before imaging, so the environment is configured
# via /etc/skel, from which the users of instantiated VMs are created.
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
gc() { gcloud --quiet ${PROJECT:+--project "$PROJECT"} "$@"; }

WORKDIR="$(mktemp -d)"
BUILD_OK=no
BUILDER_CREATED=no
FIREWALL_RULE="${BUILDER_NAME}-ssh"
FIREWALL_CREATED=no

cleanup() {
  local rc=$?
  if [[ "$BUILD_OK" != yes && "$KEEP_BUILDER_ON_FAILURE" == yes && "$BUILDER_CREATED" == yes ]]; then
    log "Build failed; keeping builder instance '$BUILDER_NAME' and firewall rule '$FIREWALL_RULE'."
    [[ -n "${BUILDER_IP:-}" ]] && echo "  ssh -i $SSH_PRIVATE_KEY_FILE $BUILDER_USERNAME@$BUILDER_IP"
    exit $rc
  fi
  if [[ "$BUILDER_CREATED" == yes ]]; then
    log "Deleting builder instance '$BUILDER_NAME'"
    gc compute instances delete "$BUILDER_NAME" --zone "$ZONE" --delete-disks=all || true
  fi
  if [[ "$FIREWALL_CREATED" == yes ]]; then
    gc compute firewall-rules delete "$FIREWALL_RULE" || true
  fi
  rm -rf "$WORKDIR"
  exit $rc
}
trap cleanup EXIT

for cmd in gcloud ssh scp ssh-keygen; do
  command -v "$cmd" >/dev/null || { echo "Required command '$cmd' not found." >&2; exit 1; }
done
[[ -n "$(gcloud auth list --filter=status:ACTIVE --format='value(account)' 2>/dev/null)" ]] \
  || { echo "Not logged into Google Cloud. Run 'gcloud auth login' first." >&2; exit 1; }
PROJECT="${PROJECT:-$(gcloud config get-value project 2>/dev/null)}"
[[ -n "$PROJECT" ]] || { echo "No project set. Set PROJECT or run 'gcloud config set project <id>'." >&2; exit 1; }

if gc compute images describe "$IMAGE_NAME" --format='value(name)' >/dev/null 2>&1; then
  echo "An image named '$IMAGE_NAME' already exists." >&2
  exit 1
fi

# --- SSH key
if [[ -z "$SSH_PUBLIC_KEY_FILE" ]]; then
  ssh-keygen -q -t ed25519 -N "" -C "$BUILDER_USERNAME" -f "$WORKDIR/id_builder"
  SSH_PUBLIC_KEY_FILE="$WORKDIR/id_builder.pub"
  SSH_PRIVATE_KEY_FILE="$WORKDIR/id_builder"
else
  SSH_PRIVATE_KEY_FILE="${SSH_PUBLIC_KEY_FILE%.pub}"
fi
SSH_OPTS=(-i "$SSH_PRIVATE_KEY_FILE" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
          -o ConnectTimeout=10 -o ServerAliveInterval=30 -o LogLevel=ERROR)
echo "$BUILDER_USERNAME:$(cat "$SSH_PUBLIC_KEY_FILE")" > "$WORKDIR/ssh-keys"

log "Project: $PROJECT, zone $ZONE"
log "Building image '$IMAGE_NAME' (family '$IMAGE_FAMILY') from $BASE_IMAGE_PROJECT/$BASE_IMAGE_FAMILY"

# --- firewall rule (restricted to the builder via network tag)
gc compute firewall-rules create "$FIREWALL_RULE" --network "$NETWORK" \
  --direction INGRESS --action ALLOW --rules tcp:22 \
  --source-ranges "$SSH_SOURCE_ADDRESS" --target-tags "$BUILDER_NAME" \
  --description "temporary: SSH access to image builder" >/dev/null
FIREWALL_CREATED=yes

# --- builder instance
log "Creating builder instance '$BUILDER_NAME' ($BUILDER_MACHINE_TYPE)"
NET_ARGS=(--network "$NETWORK")
[[ -n "$SUBNET" ]] && NET_ARGS+=(--subnet "$SUBNET")
BUILDER_CREATED=yes
gc compute instances create "$BUILDER_NAME" --zone "$ZONE" \
  --machine-type "$BUILDER_MACHINE_TYPE" \
  --image-family "$BASE_IMAGE_FAMILY" --image-project "$BASE_IMAGE_PROJECT" \
  --boot-disk-size "${BUILDER_DISK_SIZE_GB}GB" --boot-disk-type "$BUILDER_DISK_TYPE" \
  "${NET_ARGS[@]}" \
  --tags "$BUILDER_NAME" \
  --labels "$IMAGE_LABELS,temporary=true" \
  --metadata enable-oslogin=FALSE \
  --metadata-from-file "ssh-keys=$WORKDIR/ssh-keys" >/dev/null
BUILDER_IP=$(gc compute instances describe "$BUILDER_NAME" --zone "$ZONE" \
  --format='get(networkInterfaces[0].accessConfigs[0].natIP)')
log "Builder instance is at $BUILDER_IP"

log "Waiting for SSH"
for i in $(seq 1 60); do
  ssh "${SSH_OPTS[@]}" "$BUILDER_USERNAME@$BUILDER_IP" true 2>/dev/null && break
  [[ $i -eq 60 ]] && { echo "SSH did not come up." >&2; exit 1; }
  sleep 5
done

# --- run setup
log "Uploading and running setup script"
setup_script > "$WORKDIR/setup.sh"
scp "${SSH_OPTS[@]}" "$WORKDIR/setup.sh" "$BUILDER_USERNAME@$BUILDER_IP:/tmp/image-setup.sh"
REMOTE_ENV=""
for v in SD_REPO_URL SD_REPO_KEY_URL INSTALL_PACKAGES EXTRA_PACKAGES INSIGHT_SETENV; do
  REMOTE_ENV+=" $v=$(printf %q "${!v}")"
done
ssh "${SSH_OPTS[@]}" "$BUILDER_USERNAME@$BUILDER_IP" "sudo env$REMOTE_ENV bash /tmp/image-setup.sh"

# --- schedule deletion of the builder user and instance-specific state with power-off,
#     then remove the user's key from the metadata (before the deletion fires), so
#     that the guest agent does not recreate the user
log "Removing builder user and instance-specific state, powering off"
ssh "${SSH_OPTS[@]}" "$BUILDER_USERNAME@$BUILDER_IP" \
  "sudo systemd-run --on-active=60 /bin/sh -c \
     'pkill -KILL -u $BUILDER_USERNAME; userdel -rf $BUILDER_USERNAME; \
      rm -f /etc/ssh/ssh_host_* /root/.bash_history; \
      cloud-init clean --logs --machine-id; poweroff'" >/dev/null
gc compute instances remove-metadata "$BUILDER_NAME" --zone "$ZONE" --keys ssh-keys >/dev/null

for i in $(seq 1 60); do
  status=$(gc compute instances describe "$BUILDER_NAME" --zone "$ZONE" --format='value(status)')
  [[ "$status" == TERMINATED ]] && break
  [[ $i -eq 60 ]] && { echo "Builder did not power off." >&2; exit 1; }
  sleep 10
done

# --- create image
log "Creating image '$IMAGE_NAME'"
gc compute images create "$IMAGE_NAME" \
  --source-disk "$BUILDER_NAME" --source-disk-zone "$ZONE" \
  --family "$IMAGE_FAMILY" \
  --description "$IMAGE_DESCRIPTION" \
  --storage-location "$IMAGE_STORAGE_LOCATION" \
  --labels "$IMAGE_LABELS" >/dev/null

BUILD_OK=yes

log "Done. Image:"
echo "  projects/$PROJECT/global/images/$IMAGE_NAME (family '$IMAGE_FAMILY')"
cat <<EOF

Create a VM from it with e.g.:
  ./gcp-vm.sh --create <vm-name>
EOF
