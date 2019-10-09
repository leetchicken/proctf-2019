FROM archlinux/base

RUN pacman-db-upgrade
RUN pacman -Sy --noconfirm

RUN pacman -S --noconfirm  socat screen
RUN useradd spaceships
EXPOSE 3777/tcp
WORKDIR /home/service
#CMD sleep 100000000
CMD ./runserver.sh
