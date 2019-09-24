#!/bin/bash -e
#Generates ova file for the service

MY_NAME="`readlink -f "$0"`"
MY_DIR="`dirname "$MY_NAME"`"
cd "${MY_DIR}"

if [ -z "$1" ]; then
    echo "USAGE: make_service_ova.sh <service>" >&2
    exit 1
fi

SERVICE="$1"

if [[ ! "$SERVICE" =~ ^[a-zA-Z0-9_]+$ ]]; then
    echo "Wrong service name" >&2
    exit 1
fi

SERVICE_FILES="$(./get_service_files.sh "$SERVICE")"

if VBoxManage showvminfo "proctf_$SERVICE" --machinereadable &> /dev/null; then
 VBoxManage unregistervm "proctf_$SERVICE" --delete
fi

VBoxManage import oracle_master_v1.ova --vsys 0 --vmname "proctf_$SERVICE"

PORT="$((RANDOM+10000))"
VBoxManage modifyvm "proctf_$SERVICE" --natpf1 "deploy,tcp,127.0.0.2,$PORT,,22"
VBoxManage startvm "proctf_$SERVICE" --type separate

echo "Waiting SSH up"

while ! nc 127.0.0.2 "$PORT" -zw 1; do
    echo "Still waiting"
    sleep 1
done

SSH_OPTS="-o StrictHostKeyChecking=no -o CheckHostIP=no -o NoHostAuthenticationForLocalhost=yes"
SSH_OPTS="$SSH_OPTS -o BatchMode=yes -o LogLevel=ERROR -o UserKnownHostsFile=/dev/null"
SSH_OPTS="$SSH_OPTS -o ConnectTimeout=20 -o User=root"

SSH="ssh $SSH_OPTS -p $PORT "
REMOTE_HOSTNAME="$(echo "$SERVICE" | tr _ -)"
$SSH 127.0.0.2 "echo $REMOTE_HOSTNAME > /etc/hostname"
$SSH 127.0.0.2 echo "SSH CONNECTED"

echo "==========================================="
echo "Please pay attention on these copied files:"
echo "==========================================="
pushd ..
rsync -lptgodRv --cvs-exclude -e "$SSH" -- $SERVICE_FILES "127.0.0.2:/service/$SERVICE/"
popd

$SSH 127.0.0.2 "cd /service/$SERVICE; docker-compose up -d"

echo "Giving the service some time to warm up"
sleep 5

VBoxManage controlvm "proctf_$SERVICE" acpipowerbutton

while VBoxManage list runningvms | grep -q "proctf_$SERVICE"; do
    echo "Waiting for vm stop"
    sleep 5
done

echo "VM stoped, exporting"
VBoxManage modifyvm "proctf_$SERVICE" --natpf1 delete deploy
if [ -f "$SERVICE.ova" ]; then
    mv "$SERVICE.ova" "$SERVICE.ova.prev"
fi
VBoxManage export "proctf_$SERVICE" -o "$SERVICE.ova"

echo "Done"
