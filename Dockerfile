# f4-bringup toolchain image:
#   ARM GCC + Renode + Robot Framework + gdb-multiarch
#   + CMSIS core headers, STM32F4 device headers (stm32f4xx.h) and FreeRTOS-Kernel, pinned by tag.
# Base is Ubuntu 22.04 (Python 3.10): Renode's pinned psutil==5.9.3 ships prebuilt wheels for it.
FROM ubuntu:22.04

ARG RENODE_VERSION=1.15.3
ARG CMSIS_CORE_TAG=v5.9.0
ARG CMSIS_DEVICE_F4_TAG=v2.6.10
ARG FREERTOS_KERNEL_TAG=V11.2.0
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi \
        gdb-multiarch make git curl ca-certificates \
        python3 python3-pip python3-venv \
        gcc python3-dev \
    && rm -rf /var/lib/apt/lists/*

# Vendor sources (headers + FreeRTOS), pinned. .git dropped to keep the image small.
RUN mkdir -p /opt/vendor && cd /opt/vendor \
    && git clone -q --depth 1 --branch ${CMSIS_CORE_TAG}      https://github.com/STMicroelectronics/cmsis_core.git \
    && git clone -q --depth 1 --branch ${CMSIS_DEVICE_F4_TAG} https://github.com/STMicroelectronics/cmsis_device_f4.git \
    && git clone -q --depth 1 --branch ${FREERTOS_KERNEL_TAG} https://github.com/FreeRTOS/FreeRTOS-Kernel.git \
    && rm -rf */.git

ENV CMSIS_CORE=/opt/vendor/cmsis_core \
    CMSIS_DEVICE_F4=/opt/vendor/cmsis_device_f4 \
    FREERTOS_KERNEL=/opt/vendor/FreeRTOS-Kernel

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