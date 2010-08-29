#! /bin/sh

# FIXME: Find a way to run autoreconf again.
# autopoint create mess in man/po
#autoreconf -v -f --install || exit 1

#autoreconf executes the following
#autopoint --force
aclocal --force 
libtoolize --copy --force
/usr/bin/autoconf --force
/usr/bin/autoheader --force
automake --add-missing --copy --force-missing

./configure \
	CFLAGS="-O2 -Wall" \
	--enable-man \
	--enable-maintainer-mode \
	--disable-shared \
	--without-libpam \
	--with-selinux \
	"$@"
