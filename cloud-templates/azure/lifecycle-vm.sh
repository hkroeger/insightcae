#!/usr/bin/env bash
#
# lifecycle-vm.sh
#
# Creates or removes an Azure VM based on an image built by create-vm-image.sh.
#
# Usage:
#   lifecycle-vm.sh -c|--create <vm-name> [options]
#   lifecycle-vm.sh -r|--remove <vm-name> [options]
#
# Options:
#   -s, --size <vm-size>        VM size
#   -u, --user <name>           admin user name
#   -k, --ssh-key <file.pub>    public SSH key
#   -i, --image <id-or-name>    image resource ID, or name of a managed image
#   -y, --yes                   do not ask for confirmation on removal
#   -h, --help                  show this help
#
# Every VM is placed into its own resource group (default: <vm-name>-rg), so
# that removal deletes the VM together with all its disks, NIC, IP and NSG.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- Azure account / location
SUBSCRIPTION="${SUBSCRIPTION:-}"                 # empty: use current default subscription
LOCATION="${LOCATION:-westeurope}"

# --- Source image (must match the settings of create-vm-image.sh)
# Explicit image resource ID; if empty, it is determined from the settings below.
IMAGE_ID="${IMAGE_ID:-}"
IMAGE_RESOURCE_GROUP="${IMAGE_RESOURCE_GROUP:-insightcae-images}"
# If GALLERY_NAME is set, the latest version of the gallery image definition is used,
# otherwise the newest managed image whose name starts with IMAGE_NAME_PREFIX.
GALLERY_NAME="${GALLERY_NAME:-}"
GALLERY_IMAGE_DEFINITION="${GALLERY_IMAGE_DEFINITION:-insightcae-ubuntu2404}"
IMAGE_NAME_PREFIX="${IMAGE_NAME_PREFIX:-insightcae-ubuntu2404-}"
SECURITY_TYPE="${SECURITY_TYPE:-Standard}"       # must match the image

# --- VM
VM_SIZE="${VM_SIZE:-Standard_D4s_v5}"
VM_RESOURCE_GROUP="${VM_RESOURCE_GROUP:-}"       # empty: <vm-name>-rg
OS_DISK_SIZE_GB="${OS_DISK_SIZE_GB:-64}"
OS_DISK_SKU="${OS_DISK_SKU:-Premium_LRS}"
DATA_DISK_SIZE_GB="${DATA_DISK_SIZE_GB:-0}"      # 0: no data disk
DATA_DISK_SKU="${DATA_DISK_SKU:-Premium_LRS}"
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"   # empty: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-*}"    # source address allowed to reach the VM via SSH
AUTO_SHUTDOWN_TIME="${AUTO_SHUTDOWN_TIME:-}"     # e.g. "1900" (UTC) for daily auto-shutdown; empty: off
VM_TAGS="${VM_TAGS:-purpose=insightcae}"

# Tag used to recognize resource groups created by this script (protects
# against accidentally removing foreign resource groups)
MANAGED_TAG_KEY="managed-by"
MANAGED_TAG_VALUE="azure-vm.sh"


###############################################################################
# Implementation
###############################################################################

log() { echo -e "[$(date +%H:%M:%S)] $*"; }
die() { echo "Error: $*" >&2; exit 1; }
usage() {
  cat <<EOF
Usage:
  $(basename "$0") -c|--create <vm-name> [options]
  $(basename "$0") -r|--remove <vm-name> [options]

Options:
  -s, --size <vm-size>        VM size (default: $VM_SIZE)
  -u, --user <name>           admin user name (default: $ADMIN_USERNAME)
  -k, --ssh-key <file.pub>    public SSH key (default: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub)
  -i, --image <id-or-name>    image resource ID, or name of a managed image in $IMAGE_RESOURCE_GROUP
                              (default: newest '${IMAGE_NAME_PREFIX}*' image or latest gallery version)
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
    -s|--size)    VM_SIZE="$2"; shift 2 ;;
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
VM_RESOURCE_GROUP="${VM_RESOURCE_GROUP:-${VM_NAME}-rg}"

command -v az >/dev/null || die "Azure CLI 'az' not found."
az account show >/dev/null 2>&1 || die "not logged into Azure. Run 'az login' first."
if [[ -n "$SUBSCRIPTION" ]]; then az account set --subscription "$SUBSCRIPTION"; fi


resolve_image() {
  if [[ -n "$IMAGE_ID" ]]; then
    # plain name: managed image in IMAGE_RESOURCE_GROUP
    if [[ "$IMAGE_ID" != /* ]]; then
      IMAGE_ID=$(az image show -g "$IMAGE_RESOURCE_GROUP" -n "$IMAGE_ID" --query id -o tsv) \
        || die "managed image not found in '$IMAGE_RESOURCE_GROUP'"
    fi
  elif [[ -n "$GALLERY_NAME" ]]; then
    # referencing the definition selects its latest version
    IMAGE_ID=$(az sig image-definition show -g "$IMAGE_RESOURCE_GROUP" -r "$GALLERY_NAME" \
                 -i "$GALLERY_IMAGE_DEFINITION" --query id -o tsv) \
      || die "gallery image definition '$GALLERY_NAME/$GALLERY_IMAGE_DEFINITION' not found"
  else
    # image names created by create-vm-image.sh end with a sortable timestamp
    IMAGE_ID=$(az image list -g "$IMAGE_RESOURCE_GROUP" \
                 --query "[?starts_with(name, '$IMAGE_NAME_PREFIX')].[name, id]" -o tsv \
               | sort | tail -n 1 | cut -f2)
    [[ -n "$IMAGE_ID" ]] || die "no managed image '${IMAGE_NAME_PREFIX}*' found in '$IMAGE_RESOURCE_GROUP'"
  fi
}


create_vm() {
  if [[ -z "$SSH_PUBLIC_KEY_FILE" ]]; then
    for k in ~/.ssh/id_ed25519.pub ~/.ssh/id_rsa.pub; do
      [[ -f "$k" ]] && { SSH_PUBLIC_KEY_FILE="$k"; break; }
    done
  fi
  [[ -f "$SSH_PUBLIC_KEY_FILE" ]] || die "no SSH public key found; specify one with --ssh-key"

  if az group exists -n "$VM_RESOURCE_GROUP" | grep -q true; then
    az vm show -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" -o none 2>/dev/null \
      && die "VM '$VM_NAME' already exists in resource group '$VM_RESOURCE_GROUP'"
  fi

  resolve_image
  log "Creating VM '$VM_NAME' ($VM_SIZE) in '$VM_RESOURCE_GROUP' ($LOCATION)"
  log "Image: $IMAGE_ID"

  # shellcheck disable=SC2086
  az group create -n "$VM_RESOURCE_GROUP" -l "$LOCATION" \
    --tags $VM_TAGS "$MANAGED_TAG_KEY=$MANAGED_TAG_VALUE" -o none

  local disk_args=(--os-disk-size-gb "$OS_DISK_SIZE_GB" --os-disk-delete-option Delete)
  if [[ "$DATA_DISK_SIZE_GB" -gt 0 ]]; then
    disk_args+=(--storage-sku "os=$OS_DISK_SKU" "0=$DATA_DISK_SKU"
                --data-disk-sizes-gb "$DATA_DISK_SIZE_GB" --data-disk-delete-option Delete)
  else
    disk_args+=(--storage-sku "$OS_DISK_SKU")
  fi

  local ip
  # shellcheck disable=SC2086
  ip=$(az vm create \
    -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" -l "$LOCATION" \
    --image "$IMAGE_ID" \
    --size "$VM_SIZE" \
    --security-type "$SECURITY_TYPE" \
    "${disk_args[@]}" \
    --nic-delete-option Delete \
    --admin-username "$ADMIN_USERNAME" \
    --ssh-key-values "$SSH_PUBLIC_KEY_FILE" \
    --authentication-type ssh \
    --public-ip-sku Standard \
    --nsg-rule NONE \
    --tags $VM_TAGS \
    --query publicIpAddress -o tsv)

  az network nsg rule create -g "$VM_RESOURCE_GROUP" --nsg-name "${VM_NAME}NSG" \
    -n allow-ssh --priority 1000 --access Allow --protocol Tcp --direction Inbound \
    --source-address-prefixes "$SSH_SOURCE_ADDRESS" --destination-port-ranges 22 -o none

  if [[ -n "$AUTO_SHUTDOWN_TIME" ]]; then
    az vm auto-shutdown -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" --time "$AUTO_SHUTDOWN_TIME" -o none
  fi

  log "VM '$VM_NAME' is running."
  echo "  public IP: $ip"
  echo "  login:     ssh $ADMIN_USERNAME@$ip"
  if [[ "$DATA_DISK_SIZE_GB" -gt 0 ]]; then
    echo "  note: the data disk is attached unformatted (see 'lsblk' on the VM)."
  fi
}


remove_vm() {
  az group exists -n "$VM_RESOURCE_GROUP" | grep -q true \
    || die "resource group '$VM_RESOURCE_GROUP' does not exist"

  local tag
  tag=$(az group show -n "$VM_RESOURCE_GROUP" --query "tags.\"$MANAGED_TAG_KEY\"" -o tsv)
  [[ "$tag" == "$MANAGED_TAG_VALUE" ]] \
    || die "resource group '$VM_RESOURCE_GROUP' was not created by this script; not removing it"

  log "Resource group '$VM_RESOURCE_GROUP' contains:"
  az resource list -g "$VM_RESOURCE_GROUP" --query "[].[type, name]" -o tsv | sed 's/^/  /'

  if [[ "$ASSUME_YES" != yes ]]; then
    read -r -p "Permanently delete VM '$VM_NAME' and all resources above? [y/N] " answer
    [[ "$answer" =~ ^[yY]([eE][sS])?$ ]] || { echo "Aborted."; exit 1; }
  fi

  log "Deleting resource group '$VM_RESOURCE_GROUP'"
  az group delete -n "$VM_RESOURCE_GROUP" --yes
  log "VM '$VM_NAME' removed."
}


case "$MODE" in
  create) create_vm ;;
  remove) remove_vm ;;
esac
