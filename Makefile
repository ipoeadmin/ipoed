all:
	cc -o ipoed -lm /usr/lib/libradius.so ipoed.c radius.c
debug:
	cc -g -o ipoed -lm /usr/lib/libradius.so ipoed.c radius.c

clean:
	rm -rf *.o ipoed