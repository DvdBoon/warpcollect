# warpcollect - link driver for the WarpOS target of the mos2wos SDK.
#
# Flags follow the three Makefiles shipped upstream in
# Sakura-IT/WarpOS_Ports/warpcollect, which are one file differing only in
# CC and CFLAGS:
#
#   Makefile      gcc              -Wall -O3 -fomit-frame-pointer -D_LINUX_
#   Makefile_mos  ppc-morphos-gcc  -Wall -O3 -fomit-frame-pointer -s
#   Makefile_wos  ppc-morphos-gcc  -v --specs=warpup -Wall -O3 ... -s
#
# They are targets of one Makefile here, plus the LD_PATH variant that
# upstream produced by editing the source before each build.

CC          ?= gcc
MOSCC       ?= ppc-morphos-gcc
CFLAGS      ?= -Wall -O3 -fomit-frame-pointer
INSTALLDIR  ?= /gg/bin
WARPOS_LD   ?= /gg/ppc-warpos/bin/ld

all: warpcollect

# MorphOS-hosted, in the two variants the SDK installs: one driving the ld
# in /gg/bin, one driving the ld in /gg/ppc-warpos/bin.
mos: warpcollect.mos warpcollect.mos-warposld

# WarpOS-hosted.  This goes through warpcollect itself, because
# --specs=warpup names it as the link driver, so a working warpcollect has
# to be installed before this target can build.
wos: warpcollect.wos

warpcollect: warpcollect.c
	$(CC) $(CFLAGS) -D_LINUX_ -o $@ $<

warpcollect.mos: warpcollect.c
	$(MOSCC) $(CFLAGS) -s -o $@ $<

warpcollect.mos-warposld: warpcollect.c
	$(MOSCC) $(CFLAGS) -s -DLD_PATH='"$(WARPOS_LD)"' -o $@ $<

warpcollect.wos: warpcollect.c
	$(MOSCC) -v --specs=warpup $(CFLAGS) -s -o $@ $<

install: warpcollect
	cp warpcollect $(INSTALLDIR)/warpcollect

clean:
	rm -f warpcollect warpcollect.mos warpcollect.mos-warposld warpcollect.wos

.PHONY: all mos wos install clean
