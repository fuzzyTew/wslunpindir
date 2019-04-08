#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>
#include <errno.h>

#include <stdio.h>
#include <unistd.h>

static int hook(long syscall_number,
		long arg0, long arg1,
		long arg2, long arg3,
		long arg4, long arg5,
		long *result)
{
	if (syscall_number != SYS_rename)
		return 1;

	*result = syscall_no_intercept(syscall_number, arg0, arg1);
	if (*result == -EACCES)
	{
		fprintf(stderr, "rename(\"%s\",\"%s\") == -EACCES ... auto-retry in 1s ...\n", (char*)arg0, (char*)arg1);
		sleep(1);
		*result = syscall_no_intercept(syscall_number, arg0, arg1);
	}
	return 0;
}

static __attribute__((constructor)) void init()
{
	intercept_hook_point = hook;
}
