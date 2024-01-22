#! /bin/sh

autoreconf -v -f --install "$(dirname "$0")" || exit 1

CFLAGS="-O2"
CFLAGS="$CFLAGS -Wall"
CFLAGS="$CFLAGS -Wextra"
CFLAGS="$CFLAGS -Werror=implicit-function-declaration"
CFLAGS="$CFLAGS -Werror=implicit-int"
CFLAGS="$CFLAGS -Werror=incompatible-pointer-types"
CFLAGS="$CFLAGS -Werror=int-conversion"
CFLAGS="$CFLAGS -Wno-expansion-to-defined"
CFLAGS="$CFLAGS -Wno-unknown-warning-option"

"$(dirname "$0")"/configure \
	CFLAGS="$CFLAGS" \
	--enable-lastlog \
	--enable-man \
	--enable-maintainer-mode \
	--enable-shared \
	--without-libpam \
	--with-selinux \
	"$@"
