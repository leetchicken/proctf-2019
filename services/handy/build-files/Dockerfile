from scratch

add build-files/passwd /etc/passwd
add build-files/group /etc/group
add --chown=handy build/handy /app/handy
add --chown=handy build-files/handy.public_key /app/token_checker_public_key.pem
add --chown=handy build-files/empty /etc/handy/empty
add --chown=handy static /app/static
add --chown=handy templates /app/templates

workdir /app
user handy

expose 8080/tcp
volume /etc/handy

entrypoint ["./handy"]
