FROM alpine:3.23 AS build
LABEL description="Builds eatmemory"

RUN apk add --no-cache make gcc musl-dev git

WORKDIR /src
COPY . .
RUN make

FROM alpine:3.23
LABEL description="A small tool to allocate memory from the command line"
COPY --from=build /src/output/eatmemory /usr/local/bin/eatmemory
ENTRYPOINT ["/usr/local/bin/eatmemory"]
