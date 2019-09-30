#!/bin/bash -e

TEAM=${1?Syntax: ./launch_vm.sh <team_id> <vm_num> <vm_name>}
VMNUM=${2?Syntax: ./launch_vm.sh <team_id> <vm_num> <vm_name>}
VMNAME=${3?Syntax: ./launch_vm.sh <team_id> <vm_num> <vm_name>}

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

if ! VBoxManage showvminfo "$vm" &>/dev/null; then
  VBoxManage clonevm "${VMNUM}_${VMNAME}" --register --name "$vm" --basefolder="/home/vbox_drives/" --mode all
fi

if ! VBoxManage list runningvms | grep -qP "\W${vm}\W"; then
  VBoxManage modifyvm "$vm" --bridgeadapter1 "tap0"
fi

VBoxManage guestproperty set "$vm" team "${TEAM}"
VBoxManage guestproperty set "$vm" service "${VMNUM}"
VBoxManage guestproperty set "$vm" root_passwd_hash "$(cat /home/cloud/serv${VMNUM}_root_passwd_hash_team${TEAM}.txt)"

VBoxManage startvm "$vm" --type headless
