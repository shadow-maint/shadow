#include "shadowlog.h"

#include "lib/shadowlog_internal.h"

void log_set_progname(const char *progname)
{
	Prog = progname;
}

const char *log_get_progname(void)
{
	return Prog;
}

void log_set_logfd(FILE *fd)
{
	if (NULL != fd)
		shadow_logfd = fd;
	else
		shadow_logfd = stderr;
}

FILE *log_get_logfd(void)
{
	if (shadow_logfd != NULL)
		return shadow_logfd;
	return stderr;
}
