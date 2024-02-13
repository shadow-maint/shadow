ARG OS_IMAGE="alpine:latest"

FROM "${OS_IMAGE}" AS build

RUN apk add \
	autoconf \
	automake \
	bash \
	build-base \
	byacc \
	cmocka-dev \
	expect \
	gettext-dev \
        git \
	libbsd-dev \
	libeconf-dev \
	libtool \
	libxslt \
	pkgconf

COPY ./ /usr/local/src/shadow/
WORKDIR /usr/local/src/shadow/

RUN ./autogen.sh \
	--without-selinux \
	--disable-man \
	--disable-nls \
	--with-yescrypt
RUN make -Orecurse -j4
RUN bash -c "trap 'cat <tests/unit/test-suite.log >&2' ERR; make check;"
RUN make install

FROM scratch AS export
COPY --from=build /usr/local/src/shadow/config.log \
    /usr/local/src/shadow/config.h ./
