FROM amazon/opendistro-for-elasticsearch:1.1.0

RUN wget -O plugins/opendistro_security/netty-tcnative-openssl-1.1.0j-static-2.0.20.Final-fedora-linux-x86_64.jar https://bintray.com/floragunncom/netty-tcnative/download_file?file_path=netty-tcnative-openssl-1.1.0j-static-2.0.20.Final-fedora-linux-x86_64.jar

ADD elasticsearch.yml /usr/share/elasticsearch/config/elasticsearch.yml
ADD roles.yml /usr/share/elasticsearch/plugins/opendistro_security/securityconfig/roles.yml
ADD roles_mapping.yml /usr/share/elasticsearch/plugins/opendistro_security/securityconfig/roles_mapping.yml
ADD tenants.yml /usr/share/elasticsearch/plugins/opendistro_security/securityconfig/tenants.yml
ADD action_groups.yml /usr/share/elasticsearch/plugins/opendistro_security/securityconfig/action_groups.yml
