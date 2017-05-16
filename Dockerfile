FROM ubuntu:16.04
MAINTAINER Benjamin Henrion <zoobab@gmail.com>
LABEL Description="This image builds eatmemory so that you can specify the amount of MB as a docker env var." 

RUN DEBIAN_FRONTEND=noninteractive apt-get update -y -q
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y -q make gcc

ENV user eatmemory
RUN useradd -d /home/$user -m -s /bin/bash $user
RUN echo "$user ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/$user
RUN chmod 0440 /etc/sudoers.d/$user

USER $user
WORKDIR /home/$user
RUN mkdir -pv code
COPY . ./code/
RUN sudo chown $user.$user -R /home/$user/code
WORKDIR /home/$user/code
RUN make && sudo make install
