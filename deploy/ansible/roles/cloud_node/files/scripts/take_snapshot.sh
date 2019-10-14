#!/bin/bash -e

TEAM=${1?Syntax: ./take_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNUM=${2?Syntax: ./take_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNAME=${3?Syntax: ./take_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
NAME=${4?Syntax: ./take_snapshot.sh <team_id> <vm_num> <vm_name> <name>}

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

used=$(du -s "/home/vbox_drives/" | cut -f 1)
remain=$((QUOTA - used/1000/1000))

if (( remain < 0 )); then
 echo 'msg: ERR, quota exceeded'
 exit 1
fi

VBoxManage snapshot "$vm" take "$NAME" --live --uniquename Number

used=$(du -s "/home/vbox_drives/" | cut -f 1)
remain=$(( QUOTA - used/1000/1000))

if (( remain < WARN_FROM )); then
 echo "msg: warning: ${remain}GB remaining for your team"
fi
