#define _XOPEN_SOURCE 500
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syscall.h>
#include <unistd.h>

#include <libsyscall_intercept_hook_point.h>



static int hook(long syscall_number,
		long arg0, long arg1,
		long arg2, long arg3,
		long arg4, long arg5,
		long *result)
{
	int retry, start_errno;
	if (syscall_number != SYS_rename)
		return 1;

	retry = true;
	start_errno = errno;
	*result = syscall_no_intercept(syscall_number, arg0, arg1);

	while (retry)
	{
		retry = false;
		if (*result == -EACCES)
		{
			char path[PATH_MAX];
			size_t pathlen;
	
			if (NULL == realpath((char*)arg0, path))
			{
				return 0; /* error canonicalizing -> real EACCESS */
			}
			pathlen = strlen(path);
	
			{
				struct stat statbuf;
				if (stat((char*)arg0, &statbuf) != 0)
					return 0; /* couldn't stat path -> real EACCES */
				if (! S_ISDIR(statbuf.st_mode))
					return 0; /* not a directory -> real EACCES */
			}
			
			/* search for open filehandles in running processes */
			size_t procidx;
			glob_t procs = { .gl_pathc = 0 };
	
			/* enumerate processes */
			glob("/proc/*", GLOB_NOSORT, NULL, &procs);
	
			for (procidx = 0; !retry && procidx < procs.gl_pathc; ++ procidx)
			{
				size_t fdidx;
	
				char * procpath = procs.gl_pathv[procidx];
	
				/* extract pid from /proc dirname */
				unsigned long pid = atol( procpath + sizeof("/proc") );
	
				/* convert proc path to proc fd path */
				size_t fdpathbuflen = strlen(procpath) + sizeof("/fd/*");
	
				char fdpathbuf[fdpathbuflen];
				glob_t fds = { .gl_pathc = 0, .gl_offs = 1 };
	
				strncpy(fdpathbuf, procpath, fdpathbuflen);
				strncat(fdpathbuf, "/fd/*", sizeof("/fd/*"));
	
				/* enumerate open fds for proc */
				glob(fdpathbuf, GLOB_NOSORT | GLOB_DOOFFS, NULL, &fds);
				
				strncpy(fdpathbuf, procpath, fdpathbuflen);
				strncat(fdpathbuf, "/cwd", sizeof("/cwd"));
				fds.gl_pathv[0] = fdpathbuf; /* check cwd too */
	
				for (fdidx = 0; fdidx < fds.gl_pathc + fds.gl_offs; ++ fdidx)
				{
					char cmppath[PATH_MAX];
					if (NULL == realpath(fds.gl_pathv[fdidx], cmppath))
						continue;
					if (0 == strncmp(path, cmppath, pathlen) && '\0' != cmppath[pathlen])
					{
						/* open file is within dir */
						FILE * tty = fopen("/dev/tty", "w");
						fprintf(tty,
							"\n"
							"########################################################################\n"
							"# WSL Directory pinned ( https://github.com/Microsoft/WSL/issues/1529 )\n"
							"# Directory: %s\n"
							"# Process holding: %ld\n"
							"# File held: %s\n"
							"# Waiting 15 seconds, then trying again ...\n"
							"########################################################################\n",
							path, pid, cmppath);
						sleep(15);
						fprintf(tty,
							"########################################################################\n"
							"# Retrying system call ...\n"
							"########################################################################\n"
							);
						fclose(tty);
						retry = true;
						break;
					}
	
				}
	
				globfree(&fds);
			}
			
			globfree(&procs);

			/* ensure at least two tries, in case file was rapidly closed before processes were walked */
			{
				FILE * tty = fopen("/dev/tty", "w");
				if (! retry)
				{
					fprintf(tty, 
						"\n"
						"########################################################################\n"
						"# System call gave -EACCES, but no files open within dir found in /proc.\n"
						"# Directory: %s\n"
						"# Retrying system call ...\n"
						"########################################################################\n",
						path
						);
				}
				errno = start_errno;
				*result = syscall_no_intercept(syscall_number, arg0, arg1);
				if (*result != -EACCES)
				{
					fprintf(tty, 
						"\n"
						"########################################################################\n"
						"# https://github.com/Microsoft/WSL/issues/1529\n"
						"# System call gave -EACCES on first try, but not on retry.\n"
						"# Directory: %s\n"
						"########################################################################\n",
						path
						);
				}
				fclose(tty);
			}
		}
	}
	return 0;
}

static __attribute__((constructor)) void init()
{
	intercept_hook_point = hook;
}
