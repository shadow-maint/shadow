# This is a dummy Makefile.am to get automake work flawlessly,
# and also cooperate to make a distribution for `make dist'

pamd_files = \
	chfn \
	chsh \
	groupmems \
	login \
	passwd \
	su

if ACCT_TOOLS_SETUID
pamd_files += \
	chage \
	chgpasswd \
	chpasswd \
	groupadd \
	groupdel \
	groupmod \
	newusers \
	useradd \
	userdel \
	usermod
endif

if USE_PAM
pamddir = $(sysconfdir)/pam.d
pamd_DATA = $(pamd_files)
endif

EXTRA_DIST = $(pamd_files)
