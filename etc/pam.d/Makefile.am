# This is a dummy Makefile.am to get automake work flawlessly,
# and also cooperate to make a distribution for `make dist'

pamd_files = \
	passwd \
	su \
	useradd \
	chage \
	chgpasswd \
	chpasswd \
	groupadd \
	groupdel \
	groupmod \
	login \
	newusers \
	useradd \
	usermod

if USE_PAM
pamddir = $(sysconfdir)/pam.d
pamd_DATA = $(pamd_files)
endif

EXTRA_DIST = $(pamd_files)
