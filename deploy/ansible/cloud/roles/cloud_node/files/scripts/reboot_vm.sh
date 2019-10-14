#!/bin/bash -e

# go to script dir
MY_NAME="`readlink -f "$0"`"
MY_DIR="`dirname "$MY_NAME"`"
cd "${MY_DIR}"

TEAM=${1?Syntax: ./reboot_vm.sh <vm_num> <vm_name> <team_id>}
VMNUM=${2?Syntax: /reboot_vm.sh <vm_num> <vm_name> <team_id>}
VMNAME=${3?Syntax: /reboot_vm.sh <vm_num> <vm_name> <team_id>}

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

if ! VBoxManage list runningvms | grep -qP "\W${vm}\W"; then
  ./launch_vm.sh "$TEAM" "$VMNUM" "$VMNAME"
  exit 0
fi

# hack around unstable VirtualBox work
timeout 20 VBoxManage controlvm "$vm" reset || [ $? -ne 124 ] || (pkill -9 -f "VBoxHeadless --comment ${vm} --startvm"; echo "That's why nobody uses VirtualBox in clouds"; sleep 5; ./launch_vm.sh "$TEAM" "$VMNUM" "$VMNAME")
