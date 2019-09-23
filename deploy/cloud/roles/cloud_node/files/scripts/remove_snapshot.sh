#!/bin/bash -e

TEAM=${1?Syntax: ./remove_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNUM=${2?Syntax: ./remove_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNAME=${3?Syntax: ./remove_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
NAME=${4?Syntax: ./remove_snapshot.sh <team_id> <vm_num> <vm_name> <name>}

QUOTA=200
WARN_FROM=100

if ! [[ $TEAM =~ ^[0-9]+$ ]]; then
  echo "team number validation error"
  exit 1
fi

if ! [[ $VMNUM =~ ^[0-9]+$ ]]; then
  echo "vm number validation error"
  exit 1
fi

if ! [[ $VMNAME =~ ^[0-9a-zA-Z_]+$ ]]; then
  echo "vm name validation error"
  exit 1
fi

vm="${VMNUM}_${VMNAME}_team${TEAM}"

VBoxManage snapshot "$vm" delete "$NAME"

used=$(du -s "/home/vbox_drives/${vm}" | cut -f 1)
remain=$(( QUOTA - used/1000/1000))

echo "msg: warning: ${remain}GB remaining for your team"
