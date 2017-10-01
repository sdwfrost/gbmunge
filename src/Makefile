include ../Make.inc

SRCS =  gbfp.c gbmunge.c

OBJS = $(SRCS:%.c=%.o)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS-add) $(CFLAGS) -c $< -o $@

release: gbmunge

gbmunge: $(OBJS)
	rm -rf $@
	$(CC) $(LDFLAGS) $(LDFLAGS-add) -o $@ $^

clean:
	rm -f *.o gbmunge
