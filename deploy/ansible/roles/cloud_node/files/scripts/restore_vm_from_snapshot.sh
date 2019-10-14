#!/bin/bash -e

TEAM=${1?Syntax: ./restore_vm_from_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNUM=${2?Syntax: ./restore_vm_from_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
VMNAME=${3?Syntax: ./restore_vm_from_snapshot.sh <team_id> <vm_num> <vm_name> <name>}
NAME=${4?Syntax: ./restore_vm_from_snapshot.sh <team_id> <vm_num> <vm_name> <name>}

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

# check if snapshot exists
if ! VBoxManage snapshot "$vm" showvminfo "$NAME" &>/dev/null; then
 echo "msg: ERR, snapshot doesn't exist"
 exit 1
fi

VBoxManage controlvm "$vm" savestate || true

if ! VBoxManage snapshot "$vm" restore "$NAME"; then
 # something gone wrong
 echo 'msg: ERR, restore failed, trying to relaunch vm from the last saved state'
fi

# go to script dir
MY_NAME="`readlink -f "$0"`"
MY_DIR="`dirname "$MY_NAME"`"
cd "${MY_DIR}"

./launch_vm.sh "$TEAM" "$VMNUM" "$VMNAME"
