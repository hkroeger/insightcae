#!/usr/bin/env bash
#
# gcp-vm-power.sh
#
# Launches or shuts down a Google Compute Engine VM created by gcp-vm.sh.
#
# Usage:
#   gcp-vm-power.sh -l|--launch   <vm-name>
#   gcp-vm-power.sh -s|--shutdown <vm-name>
#   gcp-vm-power.sh -t|--status   <vm-name>
#
# A stopped VM accrues no compute costs (disks and static IPs are still
# billed). Without a static IP, the external IP changes on every launch.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

PROJECT="${PROJECT:-}"                           # empty: current gcloud default project
ZONE="${ZONE:-europe-west3-c}"
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"    # only used for the printed ssh command


###############################################################################
# Implementation
###############################################################################

log() { echo -e "[$(date +%H:%M:%S)] $*"; }
die() { echo "Error: $*" >&2; exit 1; }
gc() { gcloud --quiet ${PROJECT:+--project "$PROJECT"} "$@"; }

usage() {
  cat <<EOF
Usage:
  $(basename "$0") -l|--launch   <vm-name>   start the VM
  $(basename "$0") -s|--shutdown <vm-name>   stop the VM
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

command -v gcloud >/dev/null || die "Google Cloud CLI 'gcloud' not found."
[[ -n "$(gcloud auth list --filter=status:ACTIVE --format='value(account)' 2>/dev/null)" ]] \
  || die "not logged into Google Cloud. Run 'gcloud auth login' first."

gc compute instances describe "$VM_NAME" --zone "$ZONE" >/dev/null 2>&1 \
  || die "VM '$VM_NAME' not found in zone $ZONE"

power_state() {
  gc compute instances describe "$VM_NAME" --zone "$ZONE" --format='value(status)'
}

public_ip() {
  gc compute instances describe "$VM_NAME" --zone "$ZONE" \
    --format='get(networkInterfaces[0].accessConfigs[0].natIP)'
}

case "$MODE" in
  launch)
    if [[ "$(power_state)" == RUNNING ]]; then
      log "VM '$VM_NAME' is already running."
    else
      log "Launching VM '$VM_NAME'"
      gc compute instances start "$VM_NAME" --zone "$ZONE" >/dev/null
      log "VM '$VM_NAME' is running."
    fi
    ip=$(public_ip)
    echo "  public IP: $ip"
    echo "  login:     ssh $ADMIN_USERNAME@$ip"
    ;;
  shutdown)
    if [[ "$(power_state)" == TERMINATED ]]; then
      log "VM '$VM_NAME' is already shut down."
    else
      log "Shutting down VM '$VM_NAME'"
      gc compute instances stop "$VM_NAME" --zone "$ZONE" >/dev/null
      log "VM '$VM_NAME' is stopped."
    fi
    ;;
  status)
    echo "$VM_NAME: $(power_state)"
    ip=$(public_ip)
    if [[ -n "$ip" ]]; then echo "  public IP: $ip"; fi
    ;;
esac
