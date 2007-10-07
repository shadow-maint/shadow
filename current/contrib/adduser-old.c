/****
** 03/17/96
** hacked a bit more, removed unused code, cleaned up for gcc -Wall.
** --marekm
**
** 02/26/96
** modified to call shadow utils (useradd,chage,passwd) on shadowed 
** systems - Cristian Gafton, gafton@sorosis.ro
**
** 6/27/95
** shadow-adduser 1.4:
**
** now it copies the /etc/skel dir into the person's dir, 
** makes the mail folders, changed some defaults and made a 'make 
** install' just for the hell of it.
**
** Greg Gallagher
** CIN.Net
**
** 1/28/95
** shadow-adduser 1.3:
** 
** Basically a bug-fix on my additions in 1.2.  Thanx to Terry Stewart 
** (stew@texas.net) for pointing out one of the many idiotic bugs I introduced.
** It was such a stupid bug that I would have never seen it myself.
**
**                                Brandon
*****
** 01/27/95
** 
** shadow-adduser 1.2:
** I took the C source from adduser-shadow (credits are below) and made
** it a little more worthwhile.  Many small changes... Here's
** the ones I can remember:
** 
** Removed support for non-shadowed systems (if you don't have shadow,
**     use the original adduser, don't get this shadow version!)
** Added support for the correct /etc/shadow fields (Min days before
**     password change, max days before password change, Warning days,
**     and how many days from expiry date does the account go invalid)
**     The previous version just left all of those fields blank.
**     There is still one field left (expiry date for the account, period)
**     which I have left blank because I do not use it and didn't want to
**     spend any more time on this.  I'm sure someone will put it in and
**     tack another plethora of credits on here. :)
** Added in the password date field, which should always reflect the last
**     date the password was changed, for expiry purposes.  "passwd" always
**     updates this field, so the adduser program should set it up right
**     initially (or a user could keep thier initial password forever ;)
**     The number is in days since Jan 1st, 1970.
**
**                       Have fun with it, and someone please make
**                       a real version(this is still just a hack)
**                       for us all to use (and Email it to me???)
**
**                               Brandon
**                                  photon@usis.com
**
***** 
** adduser 1.0: add a new user account (For systems not using shadow)
** With a nice little interface and a will to do all the work for you.
**
** Craig Hagan
** hagan@opine.cs.umass.edu
**
** Modified to really work, look clean, and find unused uid by Chris Cappuccio
** chris@slinky.cs.umass.edu
**
*****
**
** 01/19/95
**
** FURTHER modifications to enable shadow passwd support (kludged, but
** no more so than the original)  by Dan Crowson - dcrowson@mo.net
**
** Search on DAN for all changes...
**
*****
**
** cc -O -o adduser adduser.c
** Use gcc if you have it... (political reasons beyond my control) (chris)
**
** I've gotten this program to work with success under Linux (without
** shadow) and SunOS 4.1.3. I would assume it should work pretty well
** on any system that uses no shadow. (chris)
**
** If you have no crypt() then try
** cc -DNO_CRYPT -O -o adduser adduser.c xfdes.c
** I'm not sure how login operates with no crypt()... I guess
** the same way we're doing it here.
*/

#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/stat.h>

#define DEFAULT_SHELL	"/bin/bash"  /* because BASH is your friend */
#define DEFAULT_HOME	"/home"
#define USERADD_PATH	"/usr/sbin/useradd"
#define CHAGE_PATH	"/usr/sbin/chage"
#define PASSWD_PATH	"/usr/bin/passwd"
#define DEFAULT_GROUP	100

#define DEFAULT_MAX_PASS 60
#define DEFAULT_WARN_PASS 10
/* if you use this feature, you will get a lot of complaints from users
   who rarely use their accounts :)  (something like 3 months would be
   more reasonable)  --marekm */
#define DEFAULT_USER_DIE /* 10 */ 0

void main()
{
	char foo[32];			
	char uname[9],person[32],dir[32],shell[32];
	unsigned int group,min_pass,max_pass,warn_pass,user_die;
	/* the group and uid of the new user */
	int bad=0,done=0,correct=0,gets_warning=0;
	char cmd[255];
	struct group *grp;
	
	/* flags, in order:
	* bad to see if the username is in /etc/passwd, or if strange stuff has
	* been typed if the user might be put in group 0
	* done allows the program to exit when a user has been added
	* correct loops until a password is found that isn't in /etc/passwd
	* gets_warning allows the fflush to be skipped for the first gets
	* so that output is still legible
	*/

	/* The real program starts HERE! */
  
	if(geteuid()!=0)
	{
		printf("It seems you don't have access to add a new user.  Try\n");
		printf("logging in as root or su root to gain super-user access.\n");
		exit(1);
	}
  
	/* Sanity checks
	*/
	
	if (!(grp=getgrgid(DEFAULT_GROUP))){
		printf("Error: the default group %d does not exist on this system!\n",
				DEFAULT_GROUP);
		printf("adduser must be recompiled.\n");
		exit(1);
	}; 
 
	while(!correct) {		/* loop until a "good" uname is chosen */
		while(!done) {
			printf("\nLogin to add (^C to quit): ");
			if(gets_warning)	/* if the warning was already shown */
				fflush(stdout);	/* fflush stdout, otherwise set the flag */
			else
				gets_warning=1;

			gets(uname);
			if(!strlen(uname)) {
				printf("Empty input.\n");
				done=0;
				continue;
			};

			/* what I saw here before made me think maybe I was running DOS */
			/* might this be a solution? (chris) */
			if (getpwnam(uname) != NULL) {
				printf("That name is in use, choose another.\n");
				done=0;
			} else
				done=1;
		}; /* done, we have a valid new user name */
		
		/* all set, get the rest of the stuff */
		printf("\nEditing information for new user [%s]\n",uname);
  
		printf("\nFull Name [%s]: ",uname);
		gets(person);
		if (!strlen(person)) {
			bzero(person,sizeof(person));
			strcpy(person,uname);
		};
      
		do {
			bad=0; 
			printf("GID [%d]: ",DEFAULT_GROUP);
			gets(foo);
			if (!strlen(foo))
				group=DEFAULT_GROUP;
			else
				if (isdigit (*foo)) {
					group = atoi(foo);
					if (! (grp = getgrgid (group))) {
						printf("unknown gid %s\n",foo);
						group=DEFAULT_GROUP;
						bad=1;
					};
			} else
				if ((grp = getgrnam (foo)))
					group = grp->gr_gid;
				else {
					printf("unknown group %s\n",foo);
					group=DEFAULT_GROUP;
					bad=1;
				}
			if (group==0){	/* You're not allowed to make root group users! */
				printf("Creation of root group users not allowed (must be done by hand)\n");
				group=DEFAULT_GROUP;
				bad=1;
			};
		} while(bad);


		fflush(stdin);
      
		printf("\nIf home dir ends with a / then [%s] will be appended to it\n",uname);
		printf("Home Directory [%s/%s]: ",DEFAULT_HOME,uname);
		fflush(stdout);
		gets(dir);
		if (!strlen(dir)) { /* hit return */
			sprintf(dir,"%s/%s",DEFAULT_HOME,uname);
			fflush(stdin);
		} else
			if (dir[strlen(dir)-1]=='/')
				sprintf(dir,"%s%s",dir,uname);

		printf("\nShell [%s]: ",DEFAULT_SHELL);
		fflush(stdout);
		gets(shell);
		if (!strlen(shell))
			sprintf(shell,"%s",DEFAULT_SHELL);
      
		printf("\nMin. Password Change Days [0]: ");
		gets(foo);
		min_pass=atoi(foo);
            
		printf("Max. Password Change Days [%d]: ",DEFAULT_MAX_PASS);
		gets(foo);
		if (strlen(foo) > 1)
			max_pass = atoi(foo);
		else
			max_pass = DEFAULT_MAX_PASS;
            
		printf("Password Warning Days [%d]: ",DEFAULT_WARN_PASS);
		gets(foo);
		warn_pass = atoi(foo);
		if (warn_pass==0)
			warn_pass = DEFAULT_WARN_PASS;
            
		printf("Days after Password Expiry for Account Locking [%d]: ",DEFAULT_USER_DIE);
		gets(foo);
		user_die = atoi(foo);
		if (user_die == 0)
			user_die = DEFAULT_USER_DIE;
      
		printf("\nInformation for new user [%s] [%s]:\n",uname,person);
		printf("Home directory: [%s] Shell: [%s]\n",dir,shell);
		printf("GID: [%d]\n",group);
		printf("MinPass: [%d] MaxPass: [%d] WarnPass: [%d] UserExpire: [%d]\n",
				min_pass,max_pass,warn_pass,user_die);
		printf("\nIs this correct? [y/N]: ");
		fflush(stdout);
		gets(foo);

		done=bad=correct=(foo[0]=='y'||foo[0]=='Y');

		if(bad!=1)
			printf("\nUser [%s] not added\n",uname);
    }

	bzero(cmd,sizeof(cmd));
	sprintf(cmd,"%s -g %d -d %s -s %s -c \"%s\" -m -k /etc/skel %s",
			USERADD_PATH,group,dir,shell,person,uname);
	printf("Calling useradd to add new user:\n%s\n",cmd);  
	if(system(cmd)){
		printf("User add failed!\n");
		exit(errno);
	};
	bzero(cmd,sizeof(cmd));
	sprintf(cmd,"%s -m %d -M %d -W %d -I %d %s", CHAGE_PATH,
			min_pass,max_pass,warn_pass,user_die,uname);
	printf("%s\n",cmd);
	if(system(cmd)){
		printf("There was an error setting password expire values\n");
		exit(errno);
	};
	bzero(cmd,sizeof(cmd));
	sprintf(cmd,"%s %s",PASSWD_PATH,uname);
	system(cmd);
	printf("\nDone.\n");
}

