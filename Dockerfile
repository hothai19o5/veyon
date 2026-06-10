# syntax=docker/dockerfile:1
#
# EduMonitor/Veyon reproducible build environments.
#
# Default target: linux-build (Ubuntu 24.04 + Qt 6 native Linux build deps)
# Optional targets:
#   - mxe-base: Ubuntu 24.04 + dependencies required to build MXE
#   - mxe-full: mxe-base + prebuilt MXE Qt5/MinGW toolchain for Windows cross-builds

ARG UBUNTU_VERSION=24.04

FROM ubuntu:${UBUNTU_VERSION} AS base

ARG DEBIAN_FRONTEND=noninteractive
ARG UID=1000
ARG GID=1000

LABEL org.opencontainers.image.title="EduMonitor build environment" \
      org.opencontainers.image.description="Dockerized Linux and MXE/Windows build environments for EduMonitor/Veyon" \
      org.opencontainers.image.source="https://github.com/hothai19o5/veyon"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        bash \
        bzip2 \
        ca-certificates \
        cmake \
        curl \
        dos2unix \
        dpkg-dev \
        fakeroot \
        file \
        g++ \
        gcc \
        gettext \
        git \
        make \
        ninja-build \
        patch \
        pkg-config \
        python3 \
        unzip \
        wget \
        xz-utils \
        zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN if existing_user="$(getent passwd "${UID}" | cut -d: -f1)" && [[ -n "${existing_user}" ]]; then \
        userdel -r "${existing_user}" 2>/dev/null || userdel "${existing_user}"; \
    fi && \
    if existing_group="$(getent group "${GID}" | cut -d: -f1)" && [[ -n "${existing_group}" ]]; then \
        groupdel "${existing_group}"; \
    fi && \
    groupadd --gid "${GID}" builder && \
    useradd --uid "${UID}" --gid "${GID}" --create-home --shell /bin/bash builder && \
    mkdir -p /workspace/veyon /opt/mxe && \
    chown -R builder:builder /workspace /opt/mxe

COPY docker/scripts/veyon-build-linux /usr/local/bin/veyon-build-linux
COPY docker/scripts/veyon-build-mxe /usr/local/bin/veyon-build-mxe
COPY docker/scripts/veyon-build-windows-mxe /usr/local/bin/veyon-build-windows-mxe
COPY docker/scripts/veyon-install-interception-mxe /usr/local/bin/veyon-install-interception-mxe
RUN chmod +x \
        /usr/local/bin/veyon-build-linux \
        /usr/local/bin/veyon-build-mxe \
        /usr/local/bin/veyon-build-windows-mxe \
        /usr/local/bin/veyon-install-interception-mxe

WORKDIR /workspace/veyon


FROM base AS mxe-base

USER root

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        autoconf \
        automake \
        autopoint \
        bison \
        flex \
        gperf \
        intltool \
        libffi-dev \
        libgdk-pixbuf-2.0-dev \
        libltdl-dev \
        libssl-dev \
        libtool-bin \
        libxml-parser-perl \
        lzip \
        openssl \
        p7zip-full \
        perl \
        python-is-python3 \
        python3-mako \
        python3-pip \
        ruby \
        texinfo && \
    python3 -m pip install --break-system-packages --no-cache-dir 'Mako>=0.8.0' && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    chown -R builder:builder /opt/mxe

ENV MXE_PATH=/opt/mxe \
    MXE_TARGET=x86_64-w64-mingw32.shared \
    PATH=/opt/mxe/usr/bin:${PATH}

USER builder
WORKDIR /workspace/veyon
CMD ["/bin/bash"]


FROM mxe-base AS mxe-full

ARG MXE_REPO=https://github.com/mxe/mxe.git
ARG MXE_REF=master
ARG MXE_JOBS=1
ARG MXE_TARGET=x86_64-w64-mingw32.shared

ENV MXE_REPO=${MXE_REPO} \
    MXE_REF=${MXE_REF} \
    MXE_JOBS=${MXE_JOBS} \
    MXE_TARGET=${MXE_TARGET}

RUN veyon-build-mxe


FROM base AS linux-build

ARG UBUNTU_VERSION=24.04

USER root

RUN apt-get update && \
    PROC_DEV_PACKAGE="$(apt-cache show libproc2-dev >/dev/null 2>&1 && printf '%s' libproc2-dev || printf '%s' libprocps-dev)" && \
    QCA_DEV_PACKAGE="$(apt-cache show libqca-qt6-dev >/dev/null 2>&1 && printf '%s' libqca-qt6-dev || printf '%s' libqca-qt6-2-dev)" && \
    optional_packages=() && \
    for package in \
        qt6-declarative-dev \
        qt6-httpserver-dev \
        qt6-l10n-tools \
        qt6-websockets-dev \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libswscale-dev; do \
        if apt-cache show "${package}" >/dev/null 2>&1; then \
            optional_packages+=("${package}"); \
        fi; \
    done && \
    apt-get install --no-install-recommends -y \
        qt6-base-dev \
        qt6-base-private-dev \
        qt6-base-dev-tools \
        qt6-tools-dev \
        qt6-tools-dev-tools \
        qt6-5compat-dev \
        "${QCA_DEV_PACKAGE}" \
        libqca-qt6-plugins \
        xorg-dev \
        libxtst-dev \
        libfakekey-dev \
        libjpeg-dev \
        zlib1g-dev \
        libpng-dev \
        libssl-dev \
        libpam0g-dev \
        "${PROC_DEV_PACKAGE}" \
        liblzo2-dev \
        libldap2-dev \
        libsasl2-dev \
        libvncserver-dev \
        "${optional_packages[@]}" && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV CPACK_DIST=ubuntu.${UBUNTU_VERSION} \
    CMAKE_BUILD_TYPE=RelWithDebInfo \
    CMAKE_INSTALL_PREFIX=/usr \
    WITH_TRANSLATIONS=OFF

USER builder
WORKDIR /workspace/veyon
CMD ["/bin/bash"]
