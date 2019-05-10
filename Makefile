
CFLAGS = -std=c11 -Wall -Werror -O2 -g -D_GNU_SOURCE
LDFLAGS = -lSDL2
PROGNAME = beepr

.PHONY: default all clean

default: all

all: $(PROGNAME)
	@[ -f "$(PROGNAME)" ] && ls -l --color=auto $(PROGNAME)

$(PROGNAME): beepr.c
	gcc $(CFLAGS) $(LDFLAGS) beepr.c -o $(PROGNAME)

clean:
	@rm -v $(PROGNAME) 2>/dev/null || true

