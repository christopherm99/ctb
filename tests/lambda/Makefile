SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o) lambda.o pool.o
CFLAGS := -Werror -Wall -I../.. -g -fsanitize=address
LDFLAGS := -fsanitize=address

.PHONY: clean

lambdatest: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lambda.o: ../../lambda.h
	$(CC) $(CFLAGS) \
		-DLAMBDA_IMPLEMENTATION -DLAMBDA_DEFINITIONS -DLAMBDA_USE_MPROTECT=1 \
		-x c -c $< -o $@

pool.o: ../../pool.h
	$(CC) $(CFLAGS) -DPOOL_IMPLEMENTATION -x c -c $< -o $@

clean:
	rm -f $(OBJS) lambdatest
