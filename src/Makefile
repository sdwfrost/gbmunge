include ../Make.inc

SRCS =  gbfp.c gbmunge.c

OBJS = $(SRCS:%.c=%.o)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS-add) $(CFLAGS) $(fPIC) -DNDEBUG -c $< -o $@

release debug: gbmunge

gbmunge: $(OBJS)
	rm -rf $@
	$(CC) $(LDFLAGS) $(LDFLAGS-add) -o $@ $^

clean:
	rm -f *.o gbmunge
