# f4-bringup toolchain image: ARM GCC + Renode + Robot Framework + gdb-multiarch
# Base is Ubuntu 22.04 (Python 3.10): Renode's pinned psutil==5.9.3 ships prebuilt wheels for it.
# On 24.04 (Python 3.12) pip tries to compile psutil from source and fails without gcc.
FROM ubuntu:22.04

ARG RENODE_VERSION=1.15.3
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi \
        gdb-multiarch make git curl ca-certificates \
        python3 python3-pip python3-venv \
        gcc python3-dev \
    && rm -rf /var/lib/apt/lists/*

# Renode portable (bundles its own runtime)
RUN mkdir -p /opt/renode \
    && curl -fsSL "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/renode-${RENODE_VERSION}.linux-portable.tar.gz" \
       | tar xz -C /opt/renode --strip-components=1

# Robot Framework deps used by renode-test
RUN python3 -m venv /opt/venv \
    && /opt/venv/bin/pip install --no-cache-dir --upgrade pip \
    && /opt/venv/bin/pip install --no-cache-dir -r /opt/renode/tests/requirements.txt

ENV PATH="/opt/venv/bin:/opt/renode:${PATH}"

# Best-effort: warm Renode's cache. The stock STM32F4 platform downloads an SVD file on first load.
RUN cd /opt/renode && renode --disable-xwt --plain --console \
      -e "mach create; machine LoadPlatformDescription @platforms/cpus/stm32f4.repl; quit" || true
WORKDIR /work
CMD ["bash"]
