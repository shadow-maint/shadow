ARG OS_IMAGE="fedora:latest"

FROM "${OS_IMAGE}" AS build

RUN dnf install -y dnf-plugins-core
RUN dnf builddep -y shadow-utils
RUN dnf install -y libbsd-devel libeconf-devel

COPY ./ /usr/local/src/shadow/
WORKDIR /usr/local/src/shadow/

RUN ./autogen.sh --enable-shadowgrp --enable-man --with-audit \
        --with-sha-crypt --with-bcrypt --with-yescrypt --with-selinux \
        --without-libcrack --without-libpam --enable-shared \
        --with-group-name-max-length=32
RUN make -j4
RUN make install

FROM scratch AS export
COPY --from=build /usr/local/src/shadow/config.log \
    /usr/local/src/shadow/config.h ./
