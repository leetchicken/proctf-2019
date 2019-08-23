#!/bin/sh

function get_rabbit_password_hash () {
	local SALT="$(dd if=/dev/urandom bs=4 count=1 2> /dev/null | xxd -p)"
	local PASSWORD_BYTES="$(printf "%s" $1 | xxd -p)"
	local HASH_BYTES="$(printf "%s%s" $SALT $PASSWORD_BYTES | xxd -r -p | openssl dgst -binary -sha256 | xxd -p)"
	echo "$(printf "%s%s" $SALT $HASH_BYTES | xxd -r -p | openssl base64)"
}

function get_es_password_hash () {
	local HASH="$(docker run -it --rm amazon/opendistro-for-elasticsearch:1.1.0 /bin/bash /usr/share/elasticsearch/plugins/opendistro_security/tools/hash.sh -p $1)"
	echo $HASH | sed $'s/\r//'
}

if [ ! -f definitions.json ] || [ ! -f internal_users.yml ] || [ ! -f LogProcessor.appsettings.json ] || [ ! -f IdentityServer.appsettings.json ]; then
	RABBIT_ADMIN_PASSWORD=`pwgen -Bs1 20`
	ES_ADMIN_PASSWORD=`pwgen -Bs1 20`

	cat << EOF > definitions.json
{
  "users": [
    {
      "name": "admin",
      "password_hash": "$(get_rabbit_password_hash $RABBIT_ADMIN_PASSWORD)",
      "tags": "administrator"
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
    },
    {
      "name": "errors",
      "vhost": "/",
      "type": "topic",
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
        "ElasticSearch": "https://admin:$ES_ADMIN_PASSWORD@elasticsearch:9200",
        "MongoDb": "mongodb://mongodb/users",
        "RabbitMq": "host=rabbitmq;username=admin;password=$RABBIT_ADMIN_PASSWORD"
    }
}
EOF

	cat << EOF > IdentityServer.appsettings.json
{
    "ConnectionStrings": {
        "ElasticSearch": "https://admin:$ES_ADMIN_PASSWORD@elasticsearch:9200",
        "MongoDb": "mongodb://mongodb/users",
        "RabbitMq": "host=rabbitmq;username=admin;password=$RABBIT_ADMIN_PASSWORD"
    },
    "Kestrel": {
        "EndPoints": {
            "Http": {
                "Url": "http://*:5000"
            }
        }
    },
    "Logging": {
        "LogLevel": {
          "Default": "Warning"
        }
    }
}
EOF

	cat << EOF > internal_users.yml
_meta:
  type: "internalusers"
  config_version: 2

admin:
  hash: "$(get_es_password_hash $ES_ADMIN_PASSWORD)"
  reserved: true
  backend_roles:
  - "admin"
  description: "Admin user"
EOF

else
  echo "All files already exists"
fi

docker-compose up
