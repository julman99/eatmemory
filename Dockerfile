FROM ubuntu:16.04
MAINTAINER Benjamin Henrion <zoobab@gmail.com>
LABEL Description="This image builds eatmemory so that you can specify the amount of MB as a docker env var." 

RUN DEBIAN_FRONTEND=noninteractive apt-get update -y -q
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y -q make gcc

RUN mkdir -pv /root/code
COPY . /root/code/
RUN chown root.root -R /root/code
WORKDIR /root/code
RUN make install
