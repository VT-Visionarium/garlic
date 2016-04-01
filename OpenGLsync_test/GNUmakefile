# from: https://www.opengl.org/wiki/Tutorial:_OpenGL_3.0_Context_Creation_%28GLX%29
#


CFLAGS = -g -Wall -Werror


bins = sync_test

sync_test_LDFLAGS = -lGL -lX11



build: $(bins)


%.o: %.c
	$(CC) $(CFLAGS) $($@_CPPFLAGS) -c $^ -o $@

sync_test: %: %.o
	$(CC) $(CFLAGS) $< -o $@ $($@_LDFLAGS)


install:


clean:
	rm -f *.o $(bins)

