ARG OS_IMAGE="debian:latest"

FROM "${OS_IMAGE}" AS build

RUN cat /etc/apt/sources.list.d/debian.sources
RUN sed -i 's/Types: deb/Types: deb deb-src/g' /etc/apt/sources.list.d/debian.sources
RUN export DEBIAN_PRIORITY=critical \
    && export DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y \
    && apt-get dist-upgrade -y
RUN apt-get build-dep shadow -y
RUN apt-get install \
	libltdl-dev \
	libbsd-dev \
	libcmocka-dev \
	pkgconf \
	-y

COPY ./ /usr/local/src/shadow/
WORKDIR /usr/local/src/shadow/

RUN ./autogen.sh \
	--without-selinux \
	--enable-man \
	--with-yescrypt
RUN make -kj4 || true
RUN make
RUN make check
RUN make install

FROM scratch AS export
COPY --from=build /usr/local/src/shadow/config.log \
    /usr/local/src/shadow/config.h ./
