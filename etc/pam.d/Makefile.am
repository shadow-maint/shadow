# This is a dummy Makefile.am to get automake work flawlessly,
# and also cooperate to make a distribution for `make dist'

pamd_files = \
	chpasswd \
	chfn \
	chsh \
	groupmems \
	login \
	newusers \
	passwd

if USE_PAM
pamddir = $(sysconfdir)/pam.d
pamd_DATA = $(pamd_files)
endif

if WITH_SU
pamd_files += su
endif

EXTRA_DIST = $(pamd_files)
