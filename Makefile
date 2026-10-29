# warpcollect - link driver for the mos2wos WarpOS target.
#
# Three binaries come out of one source:
#   warpcollect              host (Linux) build, drives ppc-morphos-ld
#   warpcollect.mos          MorphOS-hosted build, drives /gg/bin/ld
#   warpcollect.mos-warposld MorphOS-hosted build, drives /gg/ppc-warpos/bin/ld
#
# The MorphOS targets need the mos2wos cross-compiler on $PATH (/gg/bin).

CC          ?= gcc
MOSCC       ?= ppc-morphos-gcc
CFLAGS      ?= -O2 -Wall
WARPOS_LD   ?= /gg/ppc-warpos/bin/ld

all: warpcollect

mos: warpcollect.mos warpcollect.mos-warposld

warpcollect: warpcollect.c
	$(CC) $(CFLAGS) -D_LINUX_ -o $@ $<

warpcollect.mos: warpcollect.c
	$(MOSCC) $(CFLAGS) -o $@ $<

warpcollect.mos-warposld: warpcollect.c
	$(MOSCC) $(CFLAGS) -DLD_PATH='"$(WARPOS_LD)"' -o $@ $<

install: warpcollect
	cp warpcollect /gg/bin/warpcollect

clean:
	rm -f warpcollect warpcollect.mos warpcollect.mos-warposld

.PHONY: all mos install clean
