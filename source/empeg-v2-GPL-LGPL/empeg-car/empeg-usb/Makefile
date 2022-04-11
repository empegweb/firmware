# Makefile for empeg-car USB driver.
#
# (C) 1999-2000 empeg Ltd, http://www.empeg.com/
#
# Authors:
#  John Ripley <john@empeg.com>
#

CC		= gcc
LINUX_SRCDIR	= /usr/src/linux
KERNEL_INCLUDES	= -I$(LINUX_SRCDIR)/include -I$(LINUX_SRCDIR)/drivers/usb \
		-include $(LINUX_SRCDIR)/include/linux/modversions.h
# If you've compiled your kernel with module versions then you need this
KERNEL_FLAGS	= -D__KERNEL__ -DMODULE -DMODVERSIONS -DCPU=686
# Otherwise you need this
KERNEL_FLAGS	= -D__KERNEL__ -DMODULE -DCPU=686
CFLAGS		= -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer \
		-fno-strict-aliasing -pipe -fno-strength-reduce -march=i686

.PHONY:		all try clean

all:		empeg-car.o

empeg-car.o:	empeg-car.c
	$(CC) $(CFLAGS) $(KERNEL_FLAGS) $(KERNEL_INCLUDES) -c -o $@ $^

try:		empeg-car.o
	-rmmod empeg-car
	insmod ./empeg-car.o

clean:
	-rm -f empeg-car.o
