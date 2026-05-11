# eatmemory

A small command-line tool that allocates and **holds** a specified amount of
RAM until you release it. One job, done reliably.

```sh
eatmemory 1G
```

## Why eatmemory?

- **Verified allocation.** Every byte is written and read back through
  volatile-qualified pointers before the tool reports success. If
  `eatmemory 1G` exits with status 0, you got 1 GB of *resident, committed*
  memory. Not a lazy kernel reservation, not a write the compiler optimized
  away.
- **Portable.** Builds and runs on Linux (glibc and musl), macOS, Windows,
  and AIX. CI exercises **44 cross-compile targets**, including m68k, mips,
  ppc64le, riscv32/64, xtensa, and wasi, across **gcc 4 through gcc 13**.
  If your platform has a C99 compiler, eatmemory likely works on it.
- **Tiny, zero runtime dependencies.** A single C99 binary; no shared
  libraries beyond libc. Static-link friendly.
- **Docker-ready, multi-arch.** `docker run --rm julman99/eatmemory 1G`.
  Especially useful for exercising memory behavior *inside* containers.
- **Simple by design.** `eatmemory <size>`. For comparison:

  ```sh
  eatmemory 1G                                                    # eatmemory
  stress-ng --vm 1 --vm-bytes 1G --vm-keep --vm-method zero-one   # alternative
  ```

## Use cases

- **Test swap configuration.** Fill RAM, observe the system start paging.
- **Test container memory limits.** Run inside `docker run --memory=512m`
  and confirm the OOM-killer fires where you expect.
- **Test Kubernetes pod limits, evictions, QoS classes, and HPA.** Deploy a
  pod running `eatmemory 90%` and watch pod eviction, QoS-class behavior, or
  horizontal-pod-autoscaler triggers.
- **Validate monitoring and alerting.** Generate memory pressure on demand
  to confirm your alerts fire on the right thresholds.
- **Test service degradation under pressure.** Run alongside a real service
  and watch tail latency as the system starves for memory.
- **Test autoscaling.** Trigger memory-based scaling policies (cluster
  autoscaler, cloud VM autoscaler) on demand.
- **Chaos engineering.** Simulate a memory-hungry noisy neighbor in a
  workload.

## Installation

### From source

```sh
git clone https://github.com/julman99/eatmemory.git
cd eatmemory
sudo make install
```

Installs to `/usr/local/bin/eatmemory` by default. Override the install
location with `make install PREFIX=/opt/local`.

### macOS via Homebrew

```sh
brew tap julman99/toolbox
brew install eatmemory
```

### Docker

See [Docker](#docker) below.

## Usage

```sh
eatmemory <size>
```

Size accepts a unit suffix:

| Example          | Meaning                            |
| ---------------- | ---------------------------------- |
| `eatmemory 1024` | 1024 bytes                         |
| `eatmemory 100K` | 100 kilobytes                      |
| `eatmemory 10M`  | 10 megabytes                       |
| `eatmemory 4G`   | 4 gigabytes                        |
| `eatmemory 80%`  | 80 % of currently available memory |

### Options

| Flag              | Description                                                |
| ----------------- | ---------------------------------------------------------- |
| `-t <seconds>`    | Exit automatically after the given number of seconds.      |
| `-s <chunk-size>` | Allocate in chunks of this size (same format as `<size>`). |
| `-h`, `--help`    | Show usage and exit.                                       |

### Exit codes

| Code | Meaning                                          |
| ---- | ------------------------------------------------ |
| `0`  | Memory was allocated, verified, used, and freed. |
| `10` | Invalid `<size>` argument.                       |
| `11` | Invalid `<chunk-size>` argument.                 |
| `20` | Could not allocate the requested memory.         |
| `21` | Memory verification failed (read-back mismatch). |

## Docker

The official image is published as
[`julman99/eatmemory`](https://hub.docker.com/r/julman99/eatmemory) on
Docker Hub and supports multiple architectures.

### Eat memory inside a container

```sh
docker run -d --rm --name hungry_container julman99/eatmemory 128M
```

Check the container's memory consumption:

```
$ docker stats --no-stream=true hungry_container
CONTAINER           CPU %     MEM USAGE / LIMIT       MEM %
hungry_container    0.00%     133.9 MiB / 3.651 GiB   3.58%
```

### Test a container memory limit

```sh
docker run --rm --memory=128m julman99/eatmemory 256M
```

### Building the image

Requires Docker 17.05+ for the multi-stage build:

```sh
docker build . -t eatmemory
```

## Support this project

Bitcoin: `14LFRrMX3HmyAH9zQsnzYoVKDH6bVWiBu3`
