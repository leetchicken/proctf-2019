#!/bin/bash

./rabbitmqadmin declare exchange name=errors type=topic

curl --insecure -XPUT "https://localhost:9200/_opendistro/_security/api/roles/read_own_index" -H 'Content-Type: application/json' -d '{"cluster_permissions":["cluster_composite_ops_ro"], "index_permissions" : [{"index_patterns":["${user_name}"],"fls":[],"masked_fields":[],"allowed_actions":["read"]}]}' -u admin:admin

curl --insecure -XPUT "https://localhost:9200/_opendistro/_security/api/rolesmapping/read_own_index" -H 'Content-Type: application/json' -d '{"backend_roles":["deer_user"]}' -u admin:admin
