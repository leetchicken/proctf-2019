#!/bin/sh

function get_rabbit_password_hash () {
	local SALT="$(dd if=/dev/urandom bs=4 count=1 2> /dev/null | xxd -p)"
	local PASSWORD_BYTES="$(printf "%s" $1 | xxd -p)"
	local HASH_BYTES="$(printf "%s%s" $SALT $PASSWORD_BYTES | xxd -r -p | openssl dgst -binary -sha256 | xxd -p)"
	echo "$(printf "%s%s" $SALT $HASH_BYTES | xxd -r -p | openssl base64)"
}

if [ ! -f definitions.json ] || [ ! -f LogProcessor.appsettings.json ]; then
	RABBIT_ADMIN_PASSWORD=`pwgen -Bs1 20`
	RABBIT_LOG_PROCESSOR_PASSWORD=`pwgen -Bs1 20`

	# TODO remove
	echo "RabbitMQ admin password: $RABBIT_ADMIN_PASSWORD"
	echo "RabbitMQ log-processor password: $RABBIT_LOG_PROCESSOR_PASSWORD"

	cat << EOF > definitions.json
{
  "users": [
    {
      "name": "admin",
      "password_hash": "$(get_rabbit_password_hash $RABBIT_ADMIN_PASSWORD)",
      "tags": "administrator"
    },
    {
      "name": "log-processor",
      "password_hash": "$(get_rabbit_password_hash $RABBIT_LOG_PROCESSOR_PASSWORD)",
      "tags": ""
    }

  ],
  "vhosts": [
    {
      "name": "/"
    }
  ],
  "permissions": [
    {
      "user": "admin",
      "vhost": "/",
      "configure": ".*",
      "write": ".*",
      "read": ".*"
    },
    {
      "user": "log-processor",
      "vhost": "/",
      "configure": "^$",
      "write": "^$",
      "read": "^users\.log\-processor$"
    }

  ],
  "parameters": [],
  "policies": [],
  "exchanges": [
    {
      "name": "users",
      "vhost": "/",
      "type": "fanout",
      "durable": true,
      "auto_delete": false,
      "internal": false,
      "arguments": {}
    }
  ],
  "queues": [
    {
      "name": "users.log-processor",
      "vhost": "/",
      "durable": true,
      "auto_delete": false,
      "arguments": {}
    }
  ],
  "bindings": [
    {
      "source": "users",
      "vhost": "/",
      "destination": "users.log-processor",
      "destination_type": "queue",
      "routing_key": "*",
      "arguments": {}
    }
  ]
}
EOF

	cat << EOF > LogProcessor.appsettings.json
{
    "ConnectionStrings": {
        "RabbitMq": "host=rabbitmq;username=log-processor;password=$RABBIT_LOG_PROCESSOR_PASSWORD"
    }
}
EOF
else
  echo "All files already exists"
fi

docker-compose up
