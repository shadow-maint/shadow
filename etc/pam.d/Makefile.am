# This is a dummy Makefile.am to get automake work flawlessly,
# and also cooperate to make a distribution for `make dist'

pamd_files = \
	chage \
	chfn \
	chgpasswd \
	chpasswd \
	chsh \
	groupadd \
	groupdel \
	groupmems \
	groupmod \
	login \
	newusers \
	passwd \
	su \
	useradd \
	userdel \
	usermod

if USE_PAM
pamddir = $(sysconfdir)/pam.d
pamd_DATA = $(pamd_files)
endif

EXTRA_DIST = $(pamd_files)
