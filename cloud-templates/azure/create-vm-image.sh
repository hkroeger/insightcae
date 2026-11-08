#!/usr/bin/env bash
#
# create-vm-image.sh
#
# Builds an Azure VM image for later instantiation of VMs:
#   1. creates a temporary resource group with a small builder VM
#   2. uploads and executes the setup script embedded below (see setup_script())
#   3. deprovisions, deallocates and generalizes the builder VM
#   4. captures a managed image (and optionally publishes it to an
#      Azure Compute Gallery)
#   5. deletes the builder resource group
#
# Requirements: Azure CLI ("az"), logged in ("az login"), ssh, scp, ssh-keygen.
#
# All settings below may also be overridden from the environment, e.g.
#   IMAGE_NAME=insightcae-test LOCATION=northeurope ./create-vm-image.sh
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- Azure account / location
SUBSCRIPTION="${SUBSCRIPTION:-}"                 # empty: use current default subscription
LOCATION="${LOCATION:-westeurope}"

# --- Resulting image
IMAGE_RESOURCE_GROUP="${IMAGE_RESOURCE_GROUP:-insightcae-images}"
IMAGE_NAME="${IMAGE_NAME:-insightcae-ubuntu2404-$(date +%Y%m%d-%H%M)}"
IMAGE_TAGS="${IMAGE_TAGS:-purpose=insightcae created-by=create-vm-image.sh}"

# --- Optional: also publish into an Azure Compute Gallery (leave GALLERY_NAME empty to skip)
GALLERY_NAME="${GALLERY_NAME:-}"
GALLERY_IMAGE_DEFINITION="${GALLERY_IMAGE_DEFINITION:-insightcae-ubuntu2404}"
GALLERY_PUBLISHER="${GALLERY_PUBLISHER:-silentdynamics}"
GALLERY_OFFER="${GALLERY_OFFER:-insightcae}"
GALLERY_SKU="${GALLERY_SKU:-ubuntu2404}"
GALLERY_IMAGE_VERSION="${GALLERY_IMAGE_VERSION:-$(date +%Y.%m%d.%H%M)}"
GALLERY_REPLICA_REGIONS="${GALLERY_REPLICA_REGIONS:-$LOCATION}"

# --- Builder VM (minimal resources, only used during the build)
BUILDER_RESOURCE_GROUP="${BUILDER_RESOURCE_GROUP:-${IMAGE_NAME}-builder-rg}"
BUILDER_VM_NAME="${BUILDER_VM_NAME:-${IMAGE_NAME}-builder}"
BUILDER_VM_SIZE="${BUILDER_VM_SIZE:-Standard_B2s}"
BUILDER_OS_DISK_SIZE_GB="${BUILDER_OS_DISK_SIZE_GB:-30}"
BUILDER_OS_DISK_SKU="${BUILDER_OS_DISK_SKU:-StandardSSD_LRS}"
BASE_IMAGE="${BASE_IMAGE:-Canonical:ubuntu-24_04-lts:server:latest}"
HYPERV_GENERATION="${HYPERV_GENERATION:-V2}"
# Managed images cannot be captured from TrustedLaunch VMs, hence "Standard".
SECURITY_TYPE="${SECURITY_TYPE:-Standard}"
ADMIN_USERNAME="${ADMIN_USERNAME:-azureuser}"
# SSH key used to log into the builder; generated temporarily if empty
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"
# Source address allowed to reach the builder via SSH ("*" = any)
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-*}"

# --- Contents of the image (consumed by the embedded setup script)
SD_REPO_URL="${SD_REPO_URL:-http://downloads.silentdynamics.de/ubuntu}"
SD_REPO_KEY_URL="${SD_REPO_KEY_URL:-http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg}"
INSTALL_PACKAGES="${INSTALL_PACKAGES:-insightcae}"
EXTRA_PACKAGES="${EXTRA_PACKAGES:-htop tmux rsync}"
INSIGHT_SETENV="${INSIGHT_SETENV:-/opt/insightcae/bin/insight_setenv.sh}"

# --- Behaviour
KEEP_BUILDER_ON_FAILURE="${KEEP_BUILDER_ON_FAILURE:-no}"   # "yes": keep builder RG for debugging if something fails


###############################################################################
# Setup script executed inside the builder VM (as root)
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
# The user home of the builder is removed during deprovisioning, so the
# environment is configured via /etc/skel, from which the users of instantiated VMs are created.
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

WORKDIR="$(mktemp -d)"
BUILD_OK=no
BUILDER_RG_CREATED=no

cleanup() {
  local rc=$?
  if [[ "$BUILD_OK" != yes && "$KEEP_BUILDER_ON_FAILURE" == yes ]]; then
    log "Build failed; keeping resource group '$BUILDER_RESOURCE_GROUP' for inspection."
    [[ -n "${BUILDER_IP:-}" ]] && echo "  ssh -i $SSH_PRIVATE_KEY_FILE $ADMIN_USERNAME@$BUILDER_IP"
  elif [[ "$BUILDER_RG_CREATED" == yes ]]; then
    log "Deleting builder resource group '$BUILDER_RESOURCE_GROUP' (in background)"
    az group delete -n "$BUILDER_RESOURCE_GROUP" --yes --no-wait || true
  fi
  # keep temporary key if the builder is kept for debugging
  if [[ "$BUILD_OK" == yes || "$KEEP_BUILDER_ON_FAILURE" != yes ]]; then
    rm -rf "$WORKDIR"
  fi
  exit $rc
}
trap cleanup EXIT

for cmd in az ssh scp ssh-keygen; do
  command -v "$cmd" >/dev/null || { echo "Required command '$cmd' not found." >&2; exit 1; }
done
az account show >/dev/null 2>&1 || { echo "Not logged into Azure. Run 'az login' first." >&2; exit 1; }
[[ -n "$SUBSCRIPTION" ]] && az account set --subscription "$SUBSCRIPTION"

if az group exists -n "$BUILDER_RESOURCE_GROUP" | grep -q true; then
  echo "Builder resource group '$BUILDER_RESOURCE_GROUP' already exists; refusing to reuse it." >&2
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

log "Subscription: $(az account show --query name -o tsv)"
log "Building image '$IMAGE_NAME' in '$IMAGE_RESOURCE_GROUP' ($LOCATION) from '$BASE_IMAGE'"

# --- resource groups
az group create -n "$IMAGE_RESOURCE_GROUP" -l "$LOCATION" -o none
# shellcheck disable=SC2086
az group create -n "$BUILDER_RESOURCE_GROUP" -l "$LOCATION" --tags $IMAGE_TAGS temporary=true -o none
BUILDER_RG_CREATED=yes

# --- builder VM
log "Creating builder VM '$BUILDER_VM_NAME' ($BUILDER_VM_SIZE)"
BUILDER_IP=$(az vm create \
  -g "$BUILDER_RESOURCE_GROUP" -n "$BUILDER_VM_NAME" -l "$LOCATION" \
  --image "$BASE_IMAGE" \
  --size "$BUILDER_VM_SIZE" \
  --security-type "$SECURITY_TYPE" \
  --os-disk-size-gb "$BUILDER_OS_DISK_SIZE_GB" \
  --storage-sku "$BUILDER_OS_DISK_SKU" \
  --admin-username "$ADMIN_USERNAME" \
  --ssh-key-values "$SSH_PUBLIC_KEY_FILE" \
  --authentication-type ssh \
  --public-ip-sku Standard \
  --nsg-rule NONE \
  --query publicIpAddress -o tsv)
az network nsg rule create -g "$BUILDER_RESOURCE_GROUP" --nsg-name "${BUILDER_VM_NAME}NSG" \
  -n allow-ssh --priority 1000 --access Allow --protocol Tcp --direction Inbound \
  --source-address-prefixes "$SSH_SOURCE_ADDRESS" --destination-port-ranges 22 -o none
log "Builder VM is at $BUILDER_IP"

log "Waiting for SSH"
for i in $(seq 1 60); do
  ssh "${SSH_OPTS[@]}" "$ADMIN_USERNAME@$BUILDER_IP" true 2>/dev/null && break
  [[ $i -eq 60 ]] && { echo "SSH did not come up." >&2; exit 1; }
  sleep 5
done

# --- run setup
log "Uploading and running setup script"
setup_script > "$WORKDIR/setup.sh"
scp "${SSH_OPTS[@]}" "$WORKDIR/setup.sh" "$ADMIN_USERNAME@$BUILDER_IP:/tmp/image-setup.sh"
REMOTE_ENV=""
for v in SD_REPO_URL SD_REPO_KEY_URL INSTALL_PACKAGES EXTRA_PACKAGES INSIGHT_SETENV; do
  REMOTE_ENV+=" $v=$(printf %q "${!v}")"
done
ssh "${SSH_OPTS[@]}" "$ADMIN_USERNAME@$BUILDER_IP" "sudo env$REMOTE_ENV bash /tmp/image-setup.sh"

# --- deprovision & generalize
log "Deprovisioning builder VM"
ssh "${SSH_OPTS[@]}" "$ADMIN_USERNAME@$BUILDER_IP" \
  "sudo waagent -deprovision+user -force && sudo rm -f /root/.bash_history" || true

log "Deallocating and generalizing builder VM"
az vm deallocate -g "$BUILDER_RESOURCE_GROUP" -n "$BUILDER_VM_NAME" -o none
az vm generalize -g "$BUILDER_RESOURCE_GROUP" -n "$BUILDER_VM_NAME" -o none
VM_ID=$(az vm show -g "$BUILDER_RESOURCE_GROUP" -n "$BUILDER_VM_NAME" --query id -o tsv)

# --- capture managed image
log "Capturing managed image '$IMAGE_NAME'"
# shellcheck disable=SC2086
IMAGE_ID=$(az image create \
  -g "$IMAGE_RESOURCE_GROUP" -n "$IMAGE_NAME" -l "$LOCATION" \
  --source "$VM_ID" \
  --hyper-v-generation "$HYPERV_GENERATION" \
  --tags $IMAGE_TAGS \
  --query id -o tsv)

# --- optional: compute gallery
if [[ -n "$GALLERY_NAME" ]]; then
  log "Publishing to gallery '$GALLERY_NAME' as $GALLERY_IMAGE_DEFINITION:$GALLERY_IMAGE_VERSION"
  az sig show -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" -o none 2>/dev/null \
    || az sig create -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" -l "$LOCATION" -o none
  az sig image-definition show -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" \
      -i "$GALLERY_IMAGE_DEFINITION" -o none 2>/dev/null \
    || az sig image-definition create -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" \
         -i "$GALLERY_IMAGE_DEFINITION" -l "$LOCATION" \
         --publisher "$GALLERY_PUBLISHER" --offer "$GALLERY_OFFER" --sku "$GALLERY_SKU" \
         --os-type Linux --os-state Generalized --hyper-v-generation "$HYPERV_GENERATION" -o none
  # shellcheck disable=SC2086
  az sig image-version create -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" \
    -i "$GALLERY_IMAGE_DEFINITION" -e "$GALLERY_IMAGE_VERSION" -l "$LOCATION" \
    --managed-image "$IMAGE_ID" --target-regions $GALLERY_REPLICA_REGIONS -o none
fi

BUILD_OK=yes

log "Done. Image ID:"
echo "  $IMAGE_ID"
cat <<EOF

Create a VM from it with e.g.:
  az vm create -g <resource-group> -n <vm-name> --image "$IMAGE_ID" \\
    --size Standard_D4s_v5 --security-type $SECURITY_TYPE \\
    --admin-username <user> --ssh-key-values ~/.ssh/id_rsa.pub
EOF
