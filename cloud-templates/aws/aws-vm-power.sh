#!/usr/bin/env bash
#
# aws-vm-power.sh
#
# Launches or shuts down an AWS EC2 instance created by aws-vm.sh.
#
# Usage:
#   aws-vm-power.sh -l|--launch   <vm-name>
#   aws-vm-power.sh -s|--shutdown <vm-name>
#   aws-vm-power.sh -t|--status   <vm-name>
#
# A stopped instance accrues no compute costs (volumes and Elastic IPs are
# still billed). Without an Elastic IP, the public IP changes on every launch.
#
# All settings below may also be overridden from the environment.
#

set -euo pipefail

###############################################################################
# Settings
###############################################################################

AWS_PROFILE="${AWS_PROFILE:-}"                   # empty: default credentials
REGION="${REGION:-eu-central-1}"
ADMIN_USERNAME="${ADMIN_USERNAME:-$(id -un)}"    # only used for the printed ssh command

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

command -v aws >/dev/null || die "AWS CLI 'aws' not found."
aws sts get-caller-identity >/dev/null 2>&1 || die "no valid AWS credentials. Run 'aws configure' or 'aws sso login' first."

INSTANCE_ID=$(ec2 describe-instances \
  --filters "Name=tag:Name,Values=$VM_NAME" "Name=tag:$MANAGED_TAG_KEY,Values=$MANAGED_TAG_VALUE" \
            "Name=instance-state-name,Values=pending,running,stopping,stopped" \
  --query 'Reservations[0].Instances[0].InstanceId')
[[ -n "$INSTANCE_ID" && "$INSTANCE_ID" != None ]] || die "VM '$VM_NAME' not found in $REGION"

power_state() {
  ec2 describe-instances --instance-ids "$INSTANCE_ID" --query 'Reservations[0].Instances[0].State.Name'
}

public_ip() {
  local ip
  ip=$(ec2 describe-instances --instance-ids "$INSTANCE_ID" \
         --query 'Reservations[0].Instances[0].PublicIpAddress')
  [[ "$ip" == None ]] || echo "$ip"
}

case "$MODE" in
  launch)
    state=$(power_state)
    if [[ "$state" == running ]]; then
      log "VM '$VM_NAME' is already running."
    else
      [[ "$state" == stopping ]] && ec2 wait instance-stopped --instance-ids "$INSTANCE_ID"
      log "Launching VM '$VM_NAME' ($INSTANCE_ID)"
      ec2 start-instances --instance-ids "$INSTANCE_ID" >/dev/null
      ec2 wait instance-running --instance-ids "$INSTANCE_ID"
      log "VM '$VM_NAME' is running."
    fi
    ip=$(public_ip)
    echo "  public IP: $ip"
    echo "  login:     ssh $ADMIN_USERNAME@$ip"
    ;;
  shutdown)
    if [[ "$(power_state)" == stopped ]]; then
      log "VM '$VM_NAME' is already shut down."
    else
      log "Shutting down VM '$VM_NAME' ($INSTANCE_ID)"
      ec2 stop-instances --instance-ids "$INSTANCE_ID" >/dev/null
      ec2 wait instance-stopped --instance-ids "$INSTANCE_ID"
      log "VM '$VM_NAME' is stopped."
    fi
    ;;
  status)
    echo "$VM_NAME ($INSTANCE_ID): $(power_state)"
    ip=$(public_ip)
    if [[ -n "$ip" ]]; then echo "  public IP: $ip"; fi
    ;;
esac
