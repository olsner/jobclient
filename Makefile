CFLAGS = -Wall -Werror

all: jobclient slowecho

test: jobclient slowecho
	+./jobclient echo hello
	+./jobclient echo hello1 & ./jobclient echo hello2 & ./jobclient echo hello3 & wait
