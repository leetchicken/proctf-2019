#!/bin/bash -e
# Deploys the ova file to the cloud master host and runs them on team 10 and team 11

MY_NAME="`readlink -f "$0"`"
MY_DIR="`dirname "$MY_NAME"`"
cd "${MY_DIR}"

if [ -z "$1" ]; then
    echo "USAGE: ./deploy_to_cloud_master.sh <service>" >&2
    exit 1
fi

SERVICE="$1"
if [[ ! "$SERVICE" =~ ^[a-zA-Z0-9_]+$ ]]; then
    echo "Wrong service name" >&2
    exit 1
fi

case "$SERVICE" in
    "fraud_detector") SERVICE_NUM=1 ;; "binder") SERVICE_NUM=2   ;; "septon") SERVICE_NUM=3 ;; 
    "rubik") SERVICE_NUM=4          ;; "deer") SERVICE_NUM=5     ;; "bb") SERVICE_NUM=6 ;;
    "game_console") SERVICE_NUM=7   ;; "polyfill") SERVICE_NUM=8 ;; "convolution") SERVICE_NUM=9 ;;
    "handy") SERVICE_NUM=10         ;; "gallery") SERVICE_NUM=11 ;; "ca") SERVICE_NUM=12 ;;
    "drone_racing") SERVICE_NUM=13  ;; "startup") SERVICE_NUM=14 ;; "tracker") SERVICE_NUM=15 ;;
    "notepool") SERVICE_NUM=16      ;; "spaceships") SERVICE_NUM=17 ;; "sql_demo") SERVICE_NUM=18 ;;
    *) echo "No such service" >&2; exit 1;;
esac

eval `ssh-agent`
ssh-add cloud/roles/cloud_master/files/api_srv/proctf2019_cloud_deploy

echo "Copying ova to cloud master host"
scp $SERVICE.ova root@178.62.181.92:/root/root_cloud_scripts/

echo "Populating ova to cloud nodes"
ssh -A root@178.62.181.92 "cd /root/root_cloud_scripts/ && ./populate_ova_to_servers.py $SERVICE.ova ${SERVICE_NUM}_${SERVICE}"

for t in 10 11; do
    echo "Recreate team$t and instance"
    ssh -A root@178.62.181.92 "cd /cloud/backend/ && ./delete_team_instance.py $t $SERVICE_NUM && ./create_team_instance.py $t $SERVICE_NUM"
done

for t in 10 11; do
    echo -n "Team$t is launching on ssh root@10.60.$t.$SERVICE_NUM password:"
    cat cloud/roles/cloud_master/files/api_srv/db/team$t/serv${SERVICE_NUM}_root_passwd.txt
done
