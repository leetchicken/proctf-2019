FROM ubuntu
RUN apt update -y
RUN apt install -y openssh-server
RUN service ssh start
RUN useradd -s /bin/bash  test
RUN echo "test:sfdkjfds45a" | chpasswd
RUN mkdir -p /home/test
RUN chown test:test /home/test
COPY wrapper /home/test/wrapper
RUN chmod a+x /home/test/wrapper
ENTRYPOINT service ssh start && sleep 1000000
