#!/usr/bin/env bash
#
# power-vm.sh
#
# Launches or shuts down an Azure VM created by azure-vm.sh.
#
# Usage:
#   power-vm.sh -l|--launch   <vm-name>
#   power-vm.sh -s|--shutdown <vm-name>
#   power-vm.sh -t|--status   <vm-name>
#
# Shutdown deallocates the VM, so that no compute costs accrue while it is
# stopped (disks and the public IP are still billed). The public IP is static
# and stays the same after relaunch.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

SUBSCRIPTION="${SUBSCRIPTION:-}"                 # empty: use current default subscription
VM_RESOURCE_GROUP="${VM_RESOURCE_GROUP:-}"       # empty: <vm-name>-rg (as in azure-vm.sh)
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"    # only used for the printed ssh command


###############################################################################
# Implementation
###############################################################################

log() { echo -e "[$(date +%H:%M:%S)] $*"; }
die() { echo "Error: $*" >&2; exit 1; }
usage() {
  cat <<EOF
Usage:
  $(basename "$0") -l|--launch   <vm-name>   start the VM
  $(basename "$0") -s|--shutdown <vm-name>   stop and deallocate the VM
  $(basename "$0") -t|--status   <vm-name>   show the power state
EOF
  exit "${1:-0}"
}

MODE=""
VM_NAME=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -l|--launch)   MODE=launch;   VM_NAME="${2:-}"; shift 2 || usage 1 ;;
    -s|--shutdown) MODE=shutdown; VM_NAME="${2:-}"; shift 2 || usage 1 ;;
    -t|--status)   MODE=status;   VM_NAME="${2:-}"; shift 2 || usage 1 ;;
    -h|--help)     usage 0 ;;
    *)             echo "Unknown argument: $1" >&2; usage 1 ;;
  esac
done

[[ -n "$MODE" ]]    || { echo "One of --launch, --shutdown or --status is required." >&2; usage 1; }
[[ -n "$VM_NAME" ]] || { echo "A VM name is required." >&2; usage 1; }
VM_RESOURCE_GROUP="${VM_RESOURCE_GROUP:-${VM_NAME}-rg}"

command -v az >/dev/null || die "Azure CLI 'az' not found."
az account show >/dev/null 2>&1 || die "not logged into Azure. Run 'az login' first."
if [[ -n "$SUBSCRIPTION" ]]; then az account set --subscription "$SUBSCRIPTION"; fi

az vm show -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" -o none 2>/dev/null \
  || die "VM '$VM_NAME' not found in resource group '$VM_RESOURCE_GROUP'"

power_state() {
  az vm get-instance-view -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" \
    --query "instanceView.statuses[?starts_with(code, 'PowerState/')].displayStatus | [0]" -o tsv
}

public_ip() {
  az vm list-ip-addresses -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" \
    --query "[0].virtualMachine.network.publicIpAddresses[0].ipAddress" -o tsv
}

case "$MODE" in
  launch)
    if [[ "$(power_state)" == "VM running" ]]; then
      log "VM '$VM_NAME' is already running."
    else
      log "Launching VM '$VM_NAME'"
      az vm start -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" -o none
      log "VM '$VM_NAME' is running."
    fi
    ip=$(public_ip)
    echo "  public IP: $ip"
    echo "  login:     ssh $ADMIN_USERNAME@$ip"
    ;;
  shutdown)
    if [[ "$(power_state)" == "VM deallocated" ]]; then
      log "VM '$VM_NAME' is already shut down."
    else
      log "Shutting down and deallocating VM '$VM_NAME'"
      az vm deallocate -g "$VM_RESOURCE_GROUP" -n "$VM_NAME" -o none
      log "VM '$VM_NAME' is deallocated."
    fi
    ;;
  status)
    echo "$VM_NAME: $(power_state)"
    ip=$(public_ip)
    if [[ -n "$ip" ]]; then echo "  public IP: $ip"; fi
    ;;
esac
