## emptool (command-line downloader) Makefile
#
# (C) 1999-2000 empeg ltd.
#
# This software is licensed under the GNU General Public Licence (see file
# COPYING), unless you possess an alternative written licence from empeg ltd.
#
# (:Empeg Source Release 1.29 13-Mar-2003 18:15 rob:)
#
# Authors:
#   Mike Crowe <mac@empeg.com>
#   John Ripley <john@empeg.com>
#   Peter Hartley <peter@empeg.com>

TOP=../..

include $(TOP)/Make.rules
include $(TOP)/Make.rules.lib

NEEDS_EMPTOOL:=0

ifeq ($(BUILD), car1)
NEEDS_EMPTOOL:=1
endif
ifeq ($(BUILD), car2)
NEEDS_EMPTOOL:=1
endif
ifeq ($(ARCH), arm)
NEEDS_EMPTOOL:=0
endif

ifeq ($(NEEDS_EMPTOOL),0)
.PHONY: all

all:
	@echo emptool not needed in $(ARCH)-$(BUILD)

else

# Unlike the player, emptool uses RTTI and exceptions
TARGET_CXXFLAGS += -frtti -fexceptions -DHAVE_AUTO_PTR

# We can't use the existing TARGET_LIBS 'cos that includes the codec
TARGET_LIBS = -lempeg_protocol -lempeg_net -lempeg_tags -lvorbis -lempeg_core \
	      -lz -lpthread

ifeq ($(DEBUG), 4)
TARGET_LIBS += -lefence
endif

TARGET_SRC	= emptool.cpp empconsole.cpp playerdb.cpp database_tags.cpp node_tags.cpp \
	emptool_lookup.cpp

TARGET_MESSAGE_INPUTS = \
	$(TOP)/car-support/emptool/emptool_error.mes \
	$(TOP)/lib/core/core_errors.mes \
	$(TOP)/lib/protocol/protocol_errors.mes

TARGET_OBJS1=$(TARGET_SRC:%.c=$(TARGET_PREFIX)%.o)
TARGET_OBJS=$(TARGET_OBJS1:%.cpp=$(TARGET_PREFIX)%.o)
TARGET_DEPS=$(TARGET_OBJS:.o=.d)

.PHONY : predeps clean all static 

all:		emptool
static:		emptool.static

ifeq ($(LIBSOK),)
PREDEPS=predeps
predeps: $(TARGET_LIBS:-l%=.lib-%)
else
PREDEPS=
endif

emptool:	$(PREDEPS) $(TARGET_OBJS)
	$(TARGET_LD) $(TARGET_LDFLAGS) -o $@ $(TARGET_OBJS) $(TARGET_LIBS)

emptool.static:	$(PREDEPS) $(TARGET_OBJS)
	$(TARGET_LD) -static $(TARGET_LDFLAGS) -o $@ $(TARGET_OBJS) $(TARGET_LIBS)

idfile:		$(TARGET_PREFIX)idfile.o
	$(TARGET_LD) $(TARGET_LDFLAGS) -o $@ $^ $(TARGET_LIBS)

strip:
	$(STRIP) emptool emptool.static

.depend:	$(SRC)
	$(DEP) $(CXXFLAGS) $(SRC) > .depend

TAGS:		$(SRC) $(HEADERS)
	$(TAGS) *.[ch]

$(TOP)/lib/core/core_errors.h: $(TOP)/lib/core/core_errors.mes
	$(MAKE) -C $(TOP)/lib/core core_errors.h

emptool_error.h: emptool_error.mes
	$(ERRBUILD) -h -o emptool_error emptool_error.mes

emptool_lookup.h: $(TARGET_MESSAGE_INPUTS)
		$(ERRBUILD) -h -o emptool_lookup $(TARGET_MESSAGE_INPUTS)

emptool_lookup.cpp: emptool_lookup.h
	$(ERRBUILD) -c -o emptool_lookup $(TARGET_MESSAGE_INPUTS)

.force-header-generation: $(TOP)/lib/core/core_errors.h emptool_error.h emptool_lookup.h
	touch .force-header-generation

ifneq ($(CLEANING), 1)
-include $(TARGET_DEPS)
-include .force-header-generation
endif

endif

libclean:
	$(MAKE) -C ../../lib/core distclean
	$(MAKE) -C ../../lib/net distclean
	$(MAKE) -C ../../lib/protocol distclean

distclean : clean libclean
	$(RM) -rf $(TARGET_PREFIX)

clean: 
	-$(RM) -rf pc-*-debug* emptool emptool.static idfile
	-$(RM) emptool_error.h emptool_lookup.h emptool_lookup.cpp
	-$(RM) core *~
