FROM ubuntu
RUN apt update
RUN apt install -y python3 python3-flask python3-setuptools git
RUN apt install -y libseccomp-dev
WORKDIR /home/service
RUN git clone https://github.com/fengsp/flask-session.git && cd flask-session && python3 setup.py install
CMD ./run.sh
