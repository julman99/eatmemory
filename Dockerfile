FROM alpine:3.5 AS build
MAINTAINER julman99
LABEL Description="This image builds eatmemory"

RUN apk update
RUN apk add make gcc musl-dev

RUN mkdir -pv /root/code
COPY . /root/code/
RUN chown root.root -R /root/code
WORKDIR /root/code
RUN make

FROM alpine:3.5
MAINTAINER julman99
LABEL Description="This image runs eatmemory, a simple C program to allocate memory from the command line. Useful to test programs or systems under high memory usage conditions"
COPY --from=build /root/code/eatmemory /bin
RUN chmod +x /bin/eatmemory
ENTRYPOINT ["/bin/eatmemory"]
