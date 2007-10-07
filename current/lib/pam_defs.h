#include <security/pam_appl.h>
#include <security/pam_misc.h>

/* compatibility with different versions of Linux-PAM */
#ifndef PAM_ESTABLISH_CRED
#define PAM_ESTABLISH_CRED PAM_CRED_ESTABLISH
#endif
#ifndef PAM_DELETE_CRED
#define PAM_DELETE_CRED PAM_CRED_DELETE
#endif
#ifndef PAM_NEW_AUTHTOK_REQD
#define PAM_NEW_AUTHTOK_REQD PAM_AUTHTOKEN_REQD
#endif
#ifndef PAM_DATA_SILENT
#define PAM_DATA_SILENT 0
#endif
#ifdef PAM_STRERROR_NEEDS_TWO_ARGS  /* Linux-PAM 0.59+ */
#define PAM_STRERROR(pamh, err) pam_strerror(pamh, err)
#else
#define PAM_STRERROR(pamh, err) pam_strerror(err)
#endif
