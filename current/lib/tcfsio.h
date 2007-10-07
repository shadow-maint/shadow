struct tcfspwd {
	char tcfspass[200]; /* new password */
	char tcfsorig[200]; /* old password */
};

extern int tcfs_close(void);
extern int tcfs_file_present(void);
extern tcfspwdb *tcfs_locate(char *);
extern int tcfs_lock(void);
extern int tcfs_name(char *);
extern int tcfs_open(int);
extern int tcfs_remove(char *);
extern int tcfs_unlock(void);
extern int tcfs_update(char *, struct tcfspwd *);
