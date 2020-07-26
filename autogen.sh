#! /bin/sh

autoreconf -v -f --install || exit 1

if test -z "$NOCONFIGURE"; then
	./configure \
		CFLAGS="-O2 -Wall" \
		--enable-man \
		--enable-maintainer-mode \
		--enable-shared \
		--without-libpam \
		--with-selinux \
		"$@"
fi
