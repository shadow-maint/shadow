struct tcfspwd {
	char tcfspass[200]; /* new password */
	char tcfsorig[200]; /* old password */
};

extern int tcfs_close P_((void));
extern int tcfs_file_present P_((void));
extern tcfspwdb *tcfs_locate P_((char *));
extern int tcfs_lock P_((void));
extern int tcfs_name P_((char *));
extern int tcfs_open P_((int));
extern int tcfs_remove P_((char *));
extern int tcfs_unlock P_((void));
extern int tcfs_update P_((char *, struct tcfspwd *));
