all:
	cc -o ipoed -lm /usr/lib/libradius.so ipoed.c radius.c attrib_parser.c
debug:
	cc -g -o ipoed -lm /usr/lib/libradius.so ipoed.c radius.c attrib_parser.c

clean:
	rm -rf *.o ipoed