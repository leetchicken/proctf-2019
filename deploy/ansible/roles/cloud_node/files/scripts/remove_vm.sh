#!/bin/bash -e

TEAM=${1?Syntax: ./remove_vm.sh <team_id> <vm_num> <vm_name>}
VMNUM=${2?Syntax: ./remove_vm.sh <team_id> <vm_num> <vm_name>}
VMNAME=${3?Syntax: ./remove_vm.sh <team_id> <vm_num> <vm_name>}

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

if VBoxManage list runningvms | grep -qP "\W${vm}\W"; then
  VBoxManage controlvm "$vm" poweroff
fi

if VBoxManage showvminfo "$vm" &>/dev/null; then
  VBoxManage unregistervm "$vm" --delete
fi
