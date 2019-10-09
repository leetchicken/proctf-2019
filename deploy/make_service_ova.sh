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

VBoxManage import centos8_master_v2.ova --vsys 0 --vmname "proctf_$SERVICE"

PORT="$((RANDOM+10000))"
VBoxManage modifyvm "proctf_$SERVICE" --natpf1 "deploy,tcp,127.0.0.2,$PORT,,22"
VBoxManage startvm "proctf_$SERVICE" --type separate

echo "Waiting SSH up"

SSH_OPTS="-o StrictHostKeyChecking=no -o CheckHostIP=no -o NoHostAuthenticationForLocalhost=yes"
SSH_OPTS="$SSH_OPTS -o BatchMode=yes -o LogLevel=ERROR -o UserKnownHostsFile=/dev/null"
SSH_OPTS="$SSH_OPTS -o ConnectTimeout=5 -o User=root"
SSH="ssh $SSH_OPTS -p $PORT "

while ! $SSH 127.0.0.2 echo "SSH CONNECTED"; do
    echo "Still waiting"
    sleep 1
done

REMOTE_HOSTNAME="$(echo "$SERVICE" | tr _ -)"
$SSH 127.0.0.2 "echo $REMOTE_HOSTNAME > /etc/hostname"
$SSH 127.0.0.2 /usr/sbin/xfs_growfs -d /

echo "==========================================="
echo "Please pay attention on these copied files:"
echo "==========================================="
pushd .. > /dev/null
rsync -lptgodRv --exclude=".gitignore" -e "$SSH" -- $SERVICE_FILES "127.0.0.2:/service/$SERVICE/"
popd > /dev/null

if [ $SERVICE == "deer" ]; then
    echo "import docker images"
    docker save amazon/opendistro-for-elasticsearch:1.1.0 | $SSH 127.0.0.2 docker load
    docker save proctf/deer-elasticsearch | $SSH 127.0.0.2 docker load
    docker save proctf/deer-rabbitmq | $SSH 127.0.0.2 docker load
    docker save mongo | $SSH 127.0.0.2 docker load
    docker save proctf/deer | $SSH 127.0.0.2 docker load
fi

if [ $SERVICE == "handy" ]; then
    echo "import docker images"
    docker save mongo:4 | $SSH 127.0.0.2 docker import - mongo:4.2.0
    docker save handy | $SSH 127.0.0.2 docker import - handy
fi

if [ $SERVICE == "sql_demo" ]; then
    echo "import docker images"
    docker save proctf/sql_demo | $SSH 127.0.0.2 docker load
fi

$SSH 127.0.0.2 "cd /service/$SERVICE; docker-compose up --no-start"


RC="/etc/rc.d/rc.local"
FIRST_RUN_CMD="cd /service/$SERVICE && docker-compose up -d && sed -i /docker-compose/d $RC"
if [ -f "../services/$SERVICE/init.sh" ]; then
    echo "Found init.sh, scheduling it on the next run"
    FIRST_RUN_CMD="cd /service/$SERVICE && bash ./init.sh && docker-compose up -d && sed -i /docker-compose/d $RC"
fi
$SSH 127.0.0.2 "chmod +x $RC"
$SSH 127.0.0.2 "echo '$FIRST_RUN_CMD' >> $RC"

if [ $SERVICE == "tracker" ]; then
    $SSH 127.0.0.2 "echo 'insmod /service/tracker/dccp_modules/dccp.ko' >> $RC"
    $SSH 127.0.0.2 "echo 'insmod /service/tracker/dccp_modules/dccp_ipv4.ko' >> $RC"
fi

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
