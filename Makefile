
CFLAGS = -std=c11 -Wall -O2 -g -D_GNU_SOURCE
LDFLAGS = -lSDL2
PROGNAME = beepr

.PHONY: default all clean

default: all

all: $(PROGNAME) sdl
	@[ -f "$(PROGNAME)" ] && ls -l --color=auto $(PROGNAME) $(PROGNAME)-sdl

$(PROGNAME): beepr.c
	gcc $(CFLAGS) $(LDFLAGS) beepr.c -o $(PROGNAME)

sdl: $(PROGNAME)-sdl

$(PROGNAME)-sdl: beepr.c
	gcc $(CFLAGS) -DHAVE_SDL2 $(LDFLAGS) beepr.c -o $(PROGNAME)-sdl

clean:
	@rm -v $(PROGNAME) $(PROGNAME)-sdl 2>/dev/null || true

