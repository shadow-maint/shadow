ARG OS_IMAGE="fedora:latest"

FROM "${OS_IMAGE}" AS build

RUN dnf install -y \
	dnf-plugins-core \
	libcmocka-devel \
	systemd-devel
RUN dnf builddep -y shadow-utils

COPY ./ /usr/local/src/shadow/
WORKDIR /usr/local/src/shadow/

RUN ./autogen.sh \
	--disable-account-tools-setuid \
	--enable-lastlog \
	--enable-logind=no \
	--enable-man \
	--enable-shadowgrp \
	--enable-shared \
	--with-audit \
	--with-bcrypt \
	--with-group-name-max-length=32 \
	--with-libpam \
	--with-selinux \
	--with-sha-crypt \
	--with-yescrypt \
	--without-libbsd \
	--without-libcrack \
	--without-sssd
RUN make -Orecurse -j4
RUN bash -c "trap 'cat <tests/unit/test-suite.log >&2' ERR; make check;"
RUN make install

FROM scratch AS export
COPY --from=build /usr/local/src/shadow/config.log \
    /usr/local/src/shadow/config.h ./
