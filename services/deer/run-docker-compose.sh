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

if [ ! -f definitions.json ] || [ ! -f internal_users.yml ] || [ ! -f Deer.appsettings.json ] || [ ! -f root-ca-key.pem ] || [ ! -f root-ca.pem ] || [ ! -f esnode-key.pem ] || [ ! -f esnode.pem ] || [ ! -f rmqnode-key.pem ] || [ ! -f rmqnode.pem ] || [ ! -f deer.pfx ]  ; then
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
      "name": "errors",
      "vhost": "/",
      "type": "topic",
      "durable": true,
      "auto_delete": false,
      "internal": false,
      "arguments": {}
    }
  ],
  "queues": [],
  "bindings": []
}
EOF

	cat << EOF > Deer.appsettings.json
{
    "ConnectionStrings": {
        "ElasticSearch": "https://admin:$ES_ADMIN_PASSWORD@elasticsearch:9200",
        "MongoDb": "mongodb://mongodb/users",
        "RabbitMq": "host=rabbitmq;username=admin;password=$RABBIT_ADMIN_PASSWORD;prefetchcount=1"
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

	openssl genrsa -out root-ca-key.pem 2048
	openssl req -new -x509 -sha256 -key root-ca-key.pem -out root-ca.pem -subj "/CN=Deer CA"

	openssl genrsa -out esnode-key-temp.pem 2048
	openssl pkcs8 -inform PEM -outform PEM -in esnode-key-temp.pem -topk8 -nocrypt -v1 PBE-SHA1-3DES -out esnode-key.pem
	rm esnode-key-temp.pem
	openssl req -new -key esnode-key.pem -out esnode.csr -subj "/CN=Deer ES"
	openssl x509 -req -in esnode.csr -CA root-ca.pem -CAkey root-ca-key.pem -CAcreateserial -sha256 -out esnode.pem
	rm esnode.csr

	openssl genrsa -out rmqnode-key-temp.pem 2048
	openssl pkcs8 -inform PEM -outform PEM -in rmqnode-key-temp.pem -topk8 -nocrypt -v1 PBE-SHA1-3DES -out rmqnode-key.pem
	rm rmqnode-key-temp.pem
	openssl req -new -key rmqnode-key.pem -out rmqnode.csr -subj "/CN=Deer RMQ"
	openssl x509 -req -in rmqnode.csr -CA root-ca.pem -CAkey root-ca-key.pem -CAcreateserial -sha256 -out rmqnode.pem
	rm rmqnode.csr

	openssl genrsa -out deer-key-temp.pem 2048
	openssl pkcs8 -inform PEM -outform PEM -in deer-key-temp.pem -topk8 -nocrypt -v1 PBE-SHA1-3DES -out deer-key.pem
	rm deer-key-temp.pem
	openssl req -new -key deer-key.pem -out deer.csr -subj "/CN=Deer"
	openssl x509 -req -in deer.csr -CA root-ca.pem -CAkey root-ca-key.pem -CAcreateserial -sha256 -out deer.pem
	rm deer.csr
	openssl pkcs12 -inkey deer-key.pem -in deer.pem -export -out deer.pfx -passout pass:deer
	rm deer-key.pem deer.pem
else
  echo "All files already exists"
fi

docker-compose up
