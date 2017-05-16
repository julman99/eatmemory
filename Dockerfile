FROM alpine:3.5
MAINTAINER Benjamin Henrion <zoobab@gmail.com>
LABEL Description="This image builds eatmemory so that you can specify the amount of MB as a docker env var." 

RUN apk update
RUN apk add make gcc musl-dev

RUN mkdir -pv /root/code
COPY . /root/code/
RUN chown root.root -R /root/code
WORKDIR /root/code
RUN make
RUN cp -v eatmemory /bin
RUN chmod +x /bin/eatmemory
