ARG OS_IMAGE="debian:latest"

FROM "${OS_IMAGE}" AS build

RUN cat /etc/apt/sources.list
RUN sed -i '/deb-src/d' /etc/apt/sources.list \
    && sed -i '/^deb /p;s/ /-src /' /etc/apt/sources.list
RUN export DEBIAN_PRIORITY=critical \
    && export DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y \
    && apt-get dist-upgrade -y
RUN apt-get build-dep shadow -y
RUN apt-get install libbsd-dev pkgconf -y

COPY ./ /usr/local/src/shadow/
WORKDIR /usr/local/src/shadow/

RUN ./autogen.sh --without-selinux --enable-man --with-yescrypt
RUN make -j4
RUN make install

FROM scratch AS export
COPY --from=build /usr/local/src/shadow/config.log \
    /usr/local/src/shadow/config.h ./
