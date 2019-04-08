all: interceptrename.so

%.so: %.c
	cc $^ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -lsyscall_intercept -fpic -shared -o $@
