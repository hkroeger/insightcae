#!/usr/bin/env bash
#
# gcp-vm.sh
#
# Creates or removes a Google Compute Engine VM based on an image built by create-vm-image.sh.
#
# Usage:
#   gcp-vm.sh -c|--create <vm-name> [options]
#   gcp-vm.sh -r|--remove <vm-name> [options]
#
# Options:
#   -s, --size <machine-type>   machine type
#   -u, --user <name>           admin user name
#   -k, --ssh-key <file.pub>    public SSH key
#   -i, --image <image-name>    image name
#   -y, --yes                   do not ask for confirmation on removal
#   -h, --help                  show this help
#
# Each VM gets its own SSH firewall rule (<vm-name>-ssh) and, optionally, a
# static external IP (<vm-name>-ip). The VM is labeled managed-by=gcp-vm-sh;
# removal deletes the VM with all its disks, the firewall rule and the address.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

# --- GCP project / location
PROJECT="${PROJECT:-}"                           # empty: current gcloud default project
ZONE="${ZONE:-europe-west3-c}"

# --- Source image: explicit image name, or the latest image of IMAGE_FAMILY
IMAGE="${IMAGE:-}"
IMAGE_FAMILY="${IMAGE_FAMILY:-insightcae-ubuntu2404}"
IMAGE_PROJECT="${IMAGE_PROJECT:-}"               # empty: same as PROJECT

# --- VM
MACHINE_TYPE="${MACHINE_TYPE:-e2-standard-4}"
BOOT_DISK_SIZE_GB="${BOOT_DISK_SIZE_GB:-64}"
BOOT_DISK_TYPE="${BOOT_DISK_TYPE:-pd-balanced}"
DATA_DISK_SIZE_GB="${DATA_DISK_SIZE_GB:-0}"      # 0: no data disk
DATA_DISK_TYPE="${DATA_DISK_TYPE:-pd-balanced}"
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"
SSH_PUBLIC_KEY_FILE="${SSH_PUBLIC_KEY_FILE:-}"   # empty: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub
SSH_SOURCE_ADDRESS="${SSH_SOURCE_ADDRESS:-0.0.0.0/0}"  # CIDR allowed to reach the VM via SSH
NETWORK="${NETWORK:-default}"
SUBNET="${SUBNET:-}"                             # empty: auto-mode subnet of NETWORK
USE_STATIC_IP="${USE_STATIC_IP:-no}"             # "yes": static external IP (survives stop/start, billed separately)
AUTO_SHUTDOWN_TIME="${AUTO_SHUTDOWN_TIME:-}"     # e.g. "1900" (UTC) for daily auto-shutdown; empty: off
VM_LABELS="${VM_LABELS:-purpose=insightcae}"

# Label used to recognize VMs created by this script (protects against
# accidentally removing foreign VMs)
MANAGED_LABEL_KEY="managed-by"
MANAGED_LABEL_VALUE="gcp-vm-sh"


###############################################################################
# Implementation
###############################################################################

log() { echo -e "[$(date +%H:%M:%S)] $*"; }
die() { echo "Error: $*" >&2; exit 1; }
gc() { gcloud --quiet ${PROJECT:+--project "$PROJECT"} "$@"; }

usage() {
  cat <<EOF
Usage:
  $(basename "$0") -c|--create <vm-name> [options]
  $(basename "$0") -r|--remove <vm-name> [options]

Options:
  -s, --size <machine-type>   machine type (default: $MACHINE_TYPE)
  -u, --user <name>           admin user name (default: $ADMIN_USERNAME)
  -k, --ssh-key <file.pub>    public SSH key (default: ~/.ssh/id_ed25519.pub or ~/.ssh/id_rsa.pub)
  -i, --image <image-name>    image name (default: latest image of family '$IMAGE_FAMILY')
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
    -s|--size)    MACHINE_TYPE="$2"; shift 2 ;;
    -u|--user)    ADMIN_USERNAME="$2"; shift 2 ;;
    -k|--ssh-key) SSH_PUBLIC_KEY_FILE="$2"; shift 2 ;;
    -i|--image)   IMAGE="$2"; shift 2 ;;
    -y|--yes)     ASSUME_YES=yes; shift ;;
    -h|--help)    usage 0 ;;
    *)            echo "Unknown argument: $1" >&2; usage 1 ;;
  esac
done

[[ -n "$MODE" ]]    || { echo "Either --create or --remove is required." >&2; usage 1; }
[[ -n "$VM_NAME" ]] || { echo "A VM name is required." >&2; usage 1; }
[[ "$VM_NAME" =~ ^[a-z][a-z0-9-]{0,58}[a-z0-9]$ ]] \
  || die "invalid VM name '$VM_NAME' (lowercase letters, digits, '-'; must start with a letter)"

command -v gcloud >/dev/null || die "Google Cloud CLI 'gcloud' not found."
[[ -n "$(gcloud auth list --filter=status:ACTIVE --format='value(account)' 2>/dev/null)" ]] \
  || die "not logged into Google Cloud. Run 'gcloud auth login' first."
PROJECT="${PROJECT:-$(gcloud config get-value project 2>/dev/null)}"
[[ -n "$PROJECT" ]] || die "no project set. Set PROJECT or run 'gcloud config set project <id>'."
IMAGE_PROJECT="${IMAGE_PROJECT:-$PROJECT}"
REGION="${ZONE%-*}"
FIREWALL_RULE="${VM_NAME}-ssh"
ADDRESS_NAME="${VM_NAME}-ip"


create_vm() {
  if [[ -z "$SSH_PUBLIC_KEY_FILE" ]]; then
    for k in ~/.ssh/id_ed25519.pub ~/.ssh/id_rsa.pub; do
      [[ -f "$k" ]] && { SSH_PUBLIC_KEY_FILE="$k"; break; }
    done
  fi
  [[ -f "$SSH_PUBLIC_KEY_FILE" ]] || die "no SSH public key found; specify one with --ssh-key"
  if [[ -n "$AUTO_SHUTDOWN_TIME" ]]; then
    [[ "$AUTO_SHUTDOWN_TIME" =~ ^([01][0-9]|2[0-3])[0-5][0-9]$ ]] || die "AUTO_SHUTDOWN_TIME must be HHMM"
  fi
  gc compute instances describe "$VM_NAME" --zone "$ZONE" >/dev/null 2>&1 \
    && die "VM '$VM_NAME' already exists in zone $ZONE"

  local image_args
  if [[ -n "$IMAGE" ]]; then
    image_args=(--image "$IMAGE" --image-project "$IMAGE_PROJECT")
  else
    image_args=(--image-family "$IMAGE_FAMILY" --image-project "$IMAGE_PROJECT")
  fi

  log "Creating VM '$VM_NAME' ($MACHINE_TYPE) in $PROJECT/$ZONE"

  local tmp; tmp="$(mktemp -d)"
  echo "$ADMIN_USERNAME:$(cat "$SSH_PUBLIC_KEY_FILE")" > "$tmp/ssh-keys"
  local metadata_files=("ssh-keys=$tmp/ssh-keys")
  if [[ -n "$AUTO_SHUTDOWN_TIME" ]]; then
    cat > "$tmp/startup.sh" <<EOF
#!/bin/sh
echo "${AUTO_SHUTDOWN_TIME:2:2} ${AUTO_SHUTDOWN_TIME:0:2} * * * root /sbin/shutdown -h now" > /etc/cron.d/auto-shutdown
EOF
    metadata_files+=("startup-script=$tmp/startup.sh")
  fi
  local metadata_arg
  metadata_arg=$(IFS=,; echo "${metadata_files[*]}")

  local extra_args=(--network "$NETWORK")
  [[ -n "$SUBNET" ]] && extra_args+=(--subnet "$SUBNET")
  if [[ "$DATA_DISK_SIZE_GB" -gt 0 ]]; then
    extra_args+=(--create-disk "name=${VM_NAME}-data,size=${DATA_DISK_SIZE_GB}GB,type=${DATA_DISK_TYPE},auto-delete=yes")
  fi

  if [[ "$USE_STATIC_IP" == yes ]]; then
    gc compute addresses describe "$ADDRESS_NAME" --region "$REGION" >/dev/null 2>&1 \
      || gc compute addresses create "$ADDRESS_NAME" --region "$REGION" \
           --description "static IP of $VM_NAME, $MANAGED_LABEL_KEY=$MANAGED_LABEL_VALUE" >/dev/null
    extra_args+=(--address "$ADDRESS_NAME")
  fi

  gc compute firewall-rules describe "$FIREWALL_RULE" >/dev/null 2>&1 \
    || gc compute firewall-rules create "$FIREWALL_RULE" --network "$NETWORK" \
         --direction INGRESS --action ALLOW --rules tcp:22 \
         --source-ranges "$SSH_SOURCE_ADDRESS" --target-tags "$VM_NAME" \
         --description "SSH access to $VM_NAME, $MANAGED_LABEL_KEY=$MANAGED_LABEL_VALUE" >/dev/null

  gc compute instances create "$VM_NAME" --zone "$ZONE" \
    --machine-type "$MACHINE_TYPE" \
    "${image_args[@]}" \
    --boot-disk-size "${BOOT_DISK_SIZE_GB}GB" --boot-disk-type "$BOOT_DISK_TYPE" --boot-disk-auto-delete \
    "${extra_args[@]}" \
    --tags "$VM_NAME" \
    --labels "$VM_LABELS,$MANAGED_LABEL_KEY=$MANAGED_LABEL_VALUE" \
    --metadata enable-oslogin=FALSE \
    --metadata-from-file "$metadata_arg" >/dev/null
  rm -rf "$tmp"

  local ip
  ip=$(gc compute instances describe "$VM_NAME" --zone "$ZONE" \
         --format='get(networkInterfaces[0].accessConfigs[0].natIP)')

  log "VM '$VM_NAME' is running."
  echo "  public IP: $ip"
  echo "  login:     ssh $ADMIN_USERNAME@$ip"
  if [[ "$DATA_DISK_SIZE_GB" -gt 0 ]]; then
    echo "  note: the data disk is attached unformatted (see 'lsblk' on the VM)."
  fi
}


remove_vm() {
  local label has_vm=no has_fw=no has_ip=no
  if label=$(gc compute instances describe "$VM_NAME" --zone "$ZONE" \
               --format="value(labels.$MANAGED_LABEL_KEY)" 2>/dev/null); then
    [[ "$label" == "$MANAGED_LABEL_VALUE" ]] \
      || die "VM '$VM_NAME' was not created by this script; not removing it"
    has_vm=yes
  fi
  gc compute firewall-rules describe "$FIREWALL_RULE" --format='value(description)' 2>/dev/null \
    | grep -q "$MANAGED_LABEL_KEY=$MANAGED_LABEL_VALUE" && has_fw=yes
  gc compute addresses describe "$ADDRESS_NAME" --region "$REGION" --format='value(description)' 2>/dev/null \
    | grep -q "$MANAGED_LABEL_KEY=$MANAGED_LABEL_VALUE" && has_ip=yes
  [[ "$has_vm$has_fw$has_ip" == *yes* ]] || die "no resources of VM '$VM_NAME' found in $PROJECT/$ZONE"

  log "The following resources of VM '$VM_NAME' will be deleted:"
  [[ $has_vm == yes ]] && echo "  instance:      $VM_NAME (incl. all its disks)"
  [[ $has_fw == yes ]] && echo "  firewall rule: $FIREWALL_RULE"
  [[ $has_ip == yes ]] && echo "  static IP:     $ADDRESS_NAME"

  if [[ "$ASSUME_YES" != yes ]]; then
    read -r -p "Permanently delete these resources? [y/N] " answer
    [[ "$answer" =~ ^[yY]([eE][sS])?$ ]] || { echo "Aborted."; exit 1; }
  fi

  if [[ $has_vm == yes ]]; then
    log "Deleting instance '$VM_NAME'"
    gc compute instances delete "$VM_NAME" --zone "$ZONE" --delete-disks=all
  fi
  if [[ $has_fw == yes ]]; then
    log "Deleting firewall rule '$FIREWALL_RULE'"
    gc compute firewall-rules delete "$FIREWALL_RULE"
  fi
  if [[ $has_ip == yes ]]; then
    log "Releasing static IP '$ADDRESS_NAME'"
    gc compute addresses delete "$ADDRESS_NAME" --region "$REGION"
  fi
  log "VM '$VM_NAME' removed."
}


case "$MODE" in
  create) create_vm ;;
  remove) remove_vm ;;
esac
