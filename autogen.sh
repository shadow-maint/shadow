#! /bin/sh

autoreconf -v -f --install "$(dirname "$0")" || exit 1

CFLAGS="-O2"
CFLAGS="$CFLAGS -Wall"
CFLAGS="$CFLAGS -Wextra"
CFLAGS="$CFLAGS -Werror=discarded-qualifiers"
CFLAGS="$CFLAGS -Werror=implicit-function-declaration"
CFLAGS="$CFLAGS -Werror=implicit-int"
CFLAGS="$CFLAGS -Werror=incompatible-pointer-types"
CFLAGS="$CFLAGS -Werror=int-conversion"
CFLAGS="$CFLAGS -Werror=sign-compare"
CFLAGS="$CFLAGS -Werror=sizeof-pointer-div"
CFLAGS="$CFLAGS -Werror=stringop-overflow=4"
CFLAGS="$CFLAGS -Werror=stringop-truncation"
CFLAGS="$CFLAGS -Werror=unused-but-set-parameter"
CFLAGS="$CFLAGS -Werror=unused-function"
CFLAGS="$CFLAGS -Werror=unused-label"
CFLAGS="$CFLAGS -Werror=unused-local-typedefs"
CFLAGS="$CFLAGS -Werror=unused-parameter"
CFLAGS="$CFLAGS -Werror=unused-variable"
CFLAGS="$CFLAGS -Werror=unused-const-variable=1"
CFLAGS="$CFLAGS -Werror=unused-value"
CFLAGS="$CFLAGS -Wno-expansion-to-defined"
CFLAGS="$CFLAGS -Wno-unknown-attributes"
CFLAGS="$CFLAGS -Wno-unknown-warning-option"

"$(dirname "$0")"/configure \
	CFLAGS="$CFLAGS" \
	--enable-lastlog \
	--disable-logind \
	--enable-man \
	--enable-maintainer-mode \
	--enable-shared \
	--without-libpam \
	--with-selinux \
	"$@"
