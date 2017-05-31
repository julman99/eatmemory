eatmemory
=========

# 1. Introduction
Simple utility to allocate memory on a computer

# 2. What can I use this for?
- Test swap
- Test behaviors on a machine when there is little memory available

# 3. Installation

## Compile from sources

```
cd /tmp
git clone https://github.com/julman99/eatmemory.git
cd eatmemory
sudo make install
```

## MacOS Homebrew
```
brew tap julman99/toolbox
brew install eatmemory
```

## Using Docker

See section 5

# 4. Running

```
eatmemory <size>
```

Size is in number of bytes, megabytes or gigabytes.

## Examples

```
eatmemory 1024
eatmemory 10M
eatmemory 4G
```

# 5. Docker image

## Running a container to eat 128MB:

**eatmemory** is [available](https://hub.docker.com/r/julman99/eatmemory) in Dockerhub, so you can just run it without going
through the build process

```
$ docker run -d --rm --name hungry_container julman99/eatmemory 128M
```

Check the memory consumption of the container:

```
$ docker stats --no-stream=true hungry_container
CONTAINER           CPU %               MEM USAGE / LIMIT       MEM %               NET I/O             BLOCK I/O             PIDS
hungry_container    0.00%               133.9 MiB / 3.651 GiB   3.58%               2.01 kB / 1.08 kB   1.217 MB / 3.265 MB   4
```

## Building the container

You need at least Docker 17.05 to use the [multi-stage](https://docs.docker.com/engine/userguide/eng-image/multistage-build/) build feature

```
$ docker build . -t eatmemory
```
# 6. Support this project

Bitcoin Address: `14LFRrMX3HmyAH9zQsnzYoVKDH6bVWiBu3`
