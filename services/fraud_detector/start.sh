#!/bin/sh

export PYTHONUNBUFFERED=1

chown fraud_detector:fraud_detector /home/fraud_detector/data/rules
chown root:root /home/fraud_detector
chmod 755 /home/fraud_detector /home/fraud_detector/static /home/fraud_detector/data /home/fraud_detector/data/users
chmod 700 /home/fraud_detector/data/rules

exec gunicorn -w 16 app:application -b 0.0.0.0:10000 -u fraud_detector -g fraud_detector --capture-output --timeout=10 --reload
