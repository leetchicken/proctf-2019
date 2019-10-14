#!/bin/bash

TEAM=${1?Syntax: ./attach_vm_gui.sh <team_id> <vm_num> <vm_name> [fix]}
VMNUM=${2?Syntax: ./attach_vm_gui.sh <team_id> <vm_num> <vm_name> [fix]}
VMNAME=${3?Syntax: ./attach_vm_gui.sh <team_id> <vm_num> <vm_name> [fix]}
FIX=${4}

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

# fix keyboard layout
if [ "$FIX" == fix ]; then
 echo fixing
 setxkbmap us -print | xkbcomp - $DISPLAY
fi

VirtualBoxVM --startvm "$vm" --separate
