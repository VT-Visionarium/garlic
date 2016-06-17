CFLAGS := -g -Wall -Werror

PROGRAMS = shmIncrement

build: $(PROGRAMS)

shmIncrement: shmIncrement.o

$(PROGRAMS):
	$(CC) $(CFLAGS) $($@_LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $($@_CPPFLAGS) -c $< -o $@


install:

clean:
	rm -f *.o $(PROGRAMS)

