# empeg main Makefile
#
# (C) 1999-2001 empeg Ltd
#
# This software is licensed under the GNU General Public Licence (see file
# COPYING), unless you possess an alternative written licence from empeg ltd.
#
# (:Empeg Source Release  13-Mar-2003 18:15 rob:)
#
# Authors:
#   Mike Crowe <mac@empeg.com>
#   John Ripley <john@empeg.com>
#   Peter Hartley <peter@empeg.com>

.PHONY: zstrip dstrip bstrip rstrip strip zplayer kernel_dir player_dir lib_arch lib_arch_clean
.PHONY: lib_arm lib_arm_clean lib_pc lib_pc_clean clean player_clean pump_clean lib_clean
.PHONY: emptool
.PHONY: distclean
.PHONY: player_distclean tests_distclean
.PHONY: lib_arm_distclean
.PHONY: lib_pc_distclean
.PHONY: pump_distclean
.PHONY: autobuild
.PHONY: visuals
.PHONY: everything all autobuild

ifeq ($(MAKECMDGOALS),uberclean)

uberclean:
	[ -d player ] && $(MAKE) -C player clean BUILD=car2 || true
	[ -d lib ] && $(MAKE) -C lib clean BUILD=car2 || true
	[ -d pump ] && $(MAKE) -C pump clean BUILD=car2 || true
	[ -d car-support/emptool ] && $(MAKE) -C car-support/emptool clean BUILD=car2 ARCH=pc
	find . -name 'Debug' -o -name 'Release' -o -name '*-*-debug*' -o -name '*~' | xargs rm -rf
	rm -rf release/{car,jupiter,receiverd}
	rm -rf downloadable-root/*
	[ -d kernel ] && $(MAKE) -C kernel mrproper || true
	[ -d kernel-net ] && $(MAKE) -C kernel-net mrproper || true
	[ -d kernel-2.4 ] && $(MAKE) -C kernel-2.4 mrproper || true

else
TOP=.
include $(TOP)/Make.rules
endif

all : everything

ifeq ($(BUILD),car1)
all : pump_dir player_dir
else
ifeq ($(BUILD),car2)
all : pump_dir player_dir
else
all : player_dir
endif
endif

.PHONY: root-clean
root-clean :
	$(RM) -rf $(DISK_ROOT)
	$(RM) -rf $(RAMDISK_ROOT)

zstrip : lib_arch
	$(MAKE) -C player zstrip

dstrip : root-clean lib_arch
	$(MAKE) -C player dstrip

bstrip : lib_arch
	$(MAKE) -C player bstrip

rstrip : lib_arch
	$(MAKE) -C player rstrip

strip : lib_arch
	$(MAKE) -C player strip

zplayer : lib_arch
	$(MAKE) -C player zplayer

visuals :
	$(MAKE) -C player visuals

kernel_dir:
	$(MAKE) -C kernel

.PHONY: builder
builder : release_makeupgrade release_kernel pump_dir release_romboot downloadable-ramdisk-root

ifneq ($(ARCH), pc)
player_dir : lib_arch
	$(MAKE) -C player
else
player_dir :
endif

pump_dir : lib_pc
	$(MAKE) -C pump

lib_arch :
	$(MAKE) -C $(TOP)/lib all

lib_arch_clean :
	$(MAKE) -C $(TOP)/lib CLEANING=1 clean

lib_arch_distclean :
	$(MAKE) -C $(TOP)/lib CLEANING=1 distclean

lib_arm :
	$(MAKE) -C $(TOP)/lib ARCH=arm all

lib_arm_clean :
	$(MAKE) -C $(TOP)/lib ARCH=arm CLEANING=1 clean

lib_arm_distclean :
	$(MAKE) -C $(TOP)/lib ARCH=arm CLEANING=1 distclean

lib_pc :
	$(MAKE) -C $(TOP)/lib ARCH=pc all

lib_pc_clean :
	$(MAKE) -C $(TOP)/lib ARCH=pc CLEANING=1 clean

lib_pc_distclean :
	$(MAKE) -C $(TOP)/lib ARCH=pc CLEANING=1 distclean

clean : player_clean lib_pc_clean lib_arm_clean

player_clean :
	$(MAKE) -C player CLEANING=1 clean

player_distclean :
	$(MAKE) -C player CLEANING=1 distclean

pump_clean :
	$(MAKE) -C pump CLEANING=1 clean

pump_distclean :
	$(MAKE) -C pump CLEANING=1 distclean

emptool:
	$(MAKE) -C car-support/emptool

emptool_clean:
	$(MAKE) -C car-support/emptool CLEANING=1 clean

receiverd:
	$(MAKE) -C mercury-server/receiverd

receiverd_clean:
	$(MAKE) -C mercury-server/receiverd CLEANING=1 clean

receiverd_source_release:
	$(RM) -rf release/receiverd
	for i in mercury-server/unix mercury-server/receiverd \
		mercury-server/armgr/http_guid.h \
		mercury-server/armgr/nfs_guid.h \
		lib/net lib/util lib/core lib/db lib/model lib/vorbis \
		lib/source lib/dhcpd lib/async lib/tarfile lib/nfsd \
		lib/NgLog lib/mercury lib/berkeley_db lib/protocol \
		lib/httpd lib/thread lib/tags tools/errbuild2.py \
		car-support/playlists apps/symo Make* ; do \
		tools/copysource $$i release/receiverd/$$i ; \
	done
	tar czf receiverd.tgz -C release receiverd
	make -C release/receiverd receiverd ARCH=pc BUILD=car2
	rm -rf release/receiverd

protocol2_source_release:
	@$(RM) -rf release/protocol2
	@echo '    [[[ Copying protocol2 sources ]]]'
	@echo
	for i in lib/net lib/util/tokenparser.h lib/core lib/protocol2 \
		tools/errbuild2.py Make* ; do \
		tools/copysource $$i release/protocol2/$$i ; \
	done
	find release/protocol2 -name '*tests*' -o -name '*.dsp' -o -name '*.dsw' -o -name '*.vcproj' | xargs rm -rf
	@echo
	@echo '    [[[ Building player downloadable root ]]]'
	@echo
	$(MAKE) ARCH=pc BUILD=onyx DEBUG=0 downloadable-root
	@echo
	@echo '    [[[ Copying player downloadable root ]]]'
	@echo
	mkdir -p release/protocol2/opt
	cp -dpR downloadable-root/pc-onyx-debug0/empeg release/protocol2/opt
	mkdir -p release/protocol2/opt/empeg/{var,fids0}
	@echo
	@echo '    [[[ Creating release tarball ]]]'
	@echo
	tar cf - -C release protocol2 | gzip -9 > protocol2.tar.gz
	@echo
	@echo '    [[[ Searching for files not containing "(C)" ]]]'
	@echo
	if egrep -L "\(C\)|Copyright|public domain" `find release/protocol2/lib -type f` | egrep -v 'foo42' ; then exit 1; fi
	@echo
	@echo '    [[[ Searching for files not containing Header tag ]]]'
	@echo
	if fgrep -L "Empeg Source Release" `find release/protocol2/lib -type f`|egrep -v 'foo42' ; then exit 1; fi
	@echo
	@echo '    [[[ Testing release can be compiled ]]]'
	@echo
	make -C release/protocol2/lib/protocol2 writefid runtests BUILD=jupiter ARCH=pc > release/protocol2-out.txt 2>&1 || ( cat release/protocol2-out.txt ; exit 1 )
	@echo
	@echo '    [[[ Made protocol2.tar.gz ]]]'
	@echo
	@$(RM) -rf release/protocol2

kernel_source_release:
	@$(RM) -rf kernel-release
	@echo '    [[[ Copying kernel-2.4 ]]]'
	@echo
	tools/copysource kernel-2.4 kernel-release/linux
	@echo
	@echo '    [[[ Creating kernel tarball ]]]'
	@echo
	tar cf - -C kernel-release linux | gzip -9 > linux.tgz
	@echo
	@echo '    [[[ Testing release can be compiled ]]]'
	@echo
	make -C kernel-release/linux jupiter_config > kernel-release/out.txt 2>&1 || ( cat kernel-release/out.txt ; exit 1 )
	(yes "" | make -C kernel-release/linux oldconfig ) > kernel-release/out.txt 2>&1 || ( cat kernel-release/out.txt ; exit 1 )
	make -C kernel-release/linux zImage > kernel-release/out.txt 2>&1 || ( cat kernel-release/out.txt ; exit 1 )
	@echo
	@echo '    [[[ Made linux.tgz ]]]'
	@echo
	@$(RM) -rf kernel-release

games_source_release:
	@$(RM) -rf games-release
	@echo '    [[[ Copying games ]]]'
	@echo
	for i in games lib/SDL lib/SDL_image lib/SDL_mixer lib/SDL_input_mapper \
		lib/zlib lib/libpng lib/util/buttons.h \
		lib/util/ir_codes_jupiter.h Make* ; do \
		tools/copysource $$i games-release/$$i ; \
	done
	@echo
	@echo '    [[[ Creating games tarball ]]]'
	@echo
	tar czf games.tgz games-release
	@echo
	@echo '    [[[ Testing release can be compiled ]]]'
	@echo
	make -C games-release/games BUILD=jupiter > games-release/out.txt 2>&1 || ( cat games-release/out.txt ; exit 1 )
	@echo
	@echo '    [[[ Made games.tgz ]]]'
	@echo
	@$(RM) -rf games-release

aux_source_release:
	@$(RM) -rf aux-release
	@echo '    [[[ Copying aux ]]]'
	@echo
	for i in auxiliary Make* ; do \
		tools/copysource $$i aux-release/$$i ; \
	done
	@echo
	@echo '    [[[ Creating aux tarball ]]]'
	@echo
	tar czf auxsrc.tgz aux-release
	@echo
	@echo '    [[[ Testing release can be compiled ]]]'
	@echo
	make -C aux-release/auxiliary BUILD=jupiter > aux-release/out.txt 2>&1 || ( cat aux-release/out.txt ; exit 1 )
	@echo
	@echo '    [[[ Made auxsrc.tgz ]]]'
	@echo
	@$(RM) -rf aux-release

modutils_source_release:
	@$(RM) -rf modutils-release
	@echo '    [[[ Copying modutils ]]]'
	@echo
	for i in modules/modutils-2.4 Make* ; do \
		tools/copysource $$i modutils-release/$$i ; \
	done
	@echo
	@echo '    [[[ Creating modutils tarball ]]]'
	@echo
	tar czf modutilssrc.tgz modutils-release
	@echo
	@echo '    [[[ Testing release can be compiled ]]]'
	@echo
	make -C modutils-release/modules/modutils-2.4 -f Makefile.empeg BUILD=jupiter > modutils-release/out.txt 2>&1 || ( cat modutils-release/out.txt ; exit 1 )
	@echo
	@echo '    [[[ Made modutilssrc.tgz ]]]'
	@echo
	@$(RM) -rf modutils-release

distro_source_release:
	@echo '    [[[ Copying distro ]]]'
	@echo
	tar czf distro.tgz -C /home/empeg/website/sources stock
	@echo
	@echo '    [[[ Made distro.tgz ]]]'
	@echo

autobuild:
	$(MAKE) -C player
	$(MAKE) -C car-support/emptool
	$(MAKE) -C pump upgclient

.PHONY: tests

tests:
	$(MAKE) -C lib tests

runtests:
	$(MAKE) -C lib runtests

distclean : player_distclean lib_pc_distclean lib_arm_distclean tests_distclean

ifeq ($(ARCH), win32)
everything : release

release : release_car-support release_mercury-server

release_car-support :
	$(MAKE) -C car-support

release_mercury-server :
	$(MAKE) -C mercury-server

else # ARCH=win32

# Standard release rules used by many build types. Not every build
# needs all these rules but they make sense for every build that needs
# that component IYSWIM.

.PHONY: release_player
release_player :
	@echo -n "Making player... "
	@$(MAKE) -C player downloadable-root DISASM=1 symbols > player.log 2>&1 || ( echo "failed" ; cat player.log ; exit 1 )
	@echo done

.PHONY: release_romboot
release_romboot :
	$(MAKE) -C boot

.PHONY: release_play
release_play :
	$(MAKE) -C tests/codec downloadable-root

.PHONY: release_fonts
release_fonts :
	$(MAKE) -C fonts downloadable-root


transfer-root : downloadable-root
	$(MAKE) do-transfer-root

do-transfer-root:
	@echo Transfering to remote unit \'$(DOWNLOAD_HOST)\' over ethernet
	fakeroot "chown -R 0.0 $(DISK_ROOT) && tar cf - -C $(DISK_ROOT) ." | uclient -q ipv4=$(DOWNLOAD_HOST)/tcp=9999

transfer-cddb :
	@echo Transferring CDDB to unit \'$(DOWNLOAD_HOST)\' over ethernet - this will take some time.
	@echo "(About 23 minutes using a USB->Ethernet adapter on a Jupiter)"
	@rm -rf transfer-cddb-temp
	@mkdir -p transfer-cddb-temp/drive0/cddb
	@ln -s /home/empeg/jupiter/cddb/* transfer-cddb-temp/drive0/cddb
	@tar cf - --dereference -C transfer-cddb-temp . | uclient -q ipv4=$(DOWNLOAD_HOST)/tcp=9999
	@rm -rf transfer-cddb-temp

tar-root : downloadable-root
	@echo Compressing root ready for transfer.
	@tar cf - -C $(DISK_ROOT) . | gzip -9 > root.tar.gz

.PHONY: downloadable-ramdisk-root
downloadable-ramdisk-root :
	$(MAKE) -C pump ramdisk-root

.PHONY: release_aux
release_aux :
	$(MAKE) -C auxiliary downloadable-root

.PHONY: release_games
release_games :
	$(MAKE) -C games downloadable-root

.PHONY: release_makeupgrade
release_makeupgrade : lib_pc
	$(MAKE) -C pump ARCH=pc makeupgrade

.PHONY: release_upgclient
release_upgclient : lib_pc
	$(MAKE) -C pump ARCH=pc upgclient

.PHONY: release_libs
release_libs :
	$(MAKE) -C lib downloadable-root

.PHONY: release_module_tools
release_module_tools:
	$(MAKE) -C modules ARCH=pc modutils
	$(MAKE) -C modules ARCH=arm modutils

# This rule only works for 2.4 kernel builds, but it is common to
# Jupiter, Tardis and Whitechapel at least so we keep it available.
# First we make our separate modules and modutils, then we can make
# the kernel modules and install them. modutils must be built before
# we do modules_install.
MODULES_CONF=$(DISK_ROOT)etc/modules.conf
HOST_DEPMOD=$(shell pwd)/modules/modutils-2.4/$(HOST_PREFIX)/depmod/depmod
HOST_DEPMOD_FLAGS=-q -C $(MODULES_CONF)
KERNEL_MAKE_DEFINES=INSTALL_MOD_PATH=$(DISK_ROOT) depmod_opts_custom="$(HOST_DEPMOD_FLAGS)" DEPMOD=$(HOST_DEPMOD) 
release_modules : release_module_tools release_kernel_image
	$(MAKE) -C modules downloadable-root
	@echo -n "Making kernel modules... "
	@make -C kernel-2.4 modules $(KERNEL_MAKE_DEFINES) > modules.log 2>&1 || ( echo "failed" ; cat modules.log ; exit 1 )
	@echo done
	make -C kernel-2.4 modules_install $(KERNEL_MAKE_DEFINES)

.PHONY : transfer-modules
transfer-modules : release_modules
	@echo Transfering to remote unit \'$(DOWNLOAD_HOST)\' over ethernet
	fakeroot "chown -R 0.0 $(DISK_ROOT) && tar cf - -C $(DISK_ROOT) ." | uclient -q ipv4=$(DOWNLOAD_HOST)/tcp=9999

.PHONY: transfer-kernel
transfer-kernel : release_kernel release_modules
	@echo Transfering to remote unit \'$(DOWNLOAD_HOST)\' over ethernet
	fakeroot "chown -R 0.0 $(DISK_ROOT) && tar cf - -C $(DISK_ROOT) ." | uclient -q ipv4=$(DOWNLOAD_HOST)/tcp=9999



ifeq ($(BUILD_BASIC), car)
everything : release production_test

# This target is used by the buildcar script to build the fonts, init,
# pump binary and player.
release : release_init release_makeupgrade release_upgclient release_kernel release_romboot downloadable-root downloadable-ramdisk-root

downloadable-root : release_player release_fonts release_libs

release_init :
	$(MAKE) -C car-boot downloadable-root

.PHONY: release_kernel
release_kernel :
	$(MAKE) -C kernel zImage

# The production test is currently disabled because it is having
# interesting build problems.
production_test :
ifeq ($(BUILD), car2)
#	$(MAKE) -C mk2-test
endif
endif

tests_distclean :
	$(MAKE) -C tests CLEANING=1 distclean

ifeq ($(BUILD), mercury)
.PHONY: everything
everything : release

.PHONY: release
release : mercury_kernel mercury_init release_player release_fonts release_libs

.PHONY: mercury_init
mercury_init :
	$(MAKE) -C mercury-boot downloadable-root

.PHONY: mercury_kernel
mercury_kernel : mercury_modreloc
	$(MAKE) -C kernel-net zImage
	(cd kernel-net && ./fixup-iline)
	mkdir -p $(DISK_ROOT)
	cp kernel-net/arch/arm/boot/zImage $(DISK_ROOT)zImage
	cp kernel-net/il-binary.o $(DISK_ROOT)il-binary.o

.PHONY: mercury_modreloc
mercury_modreloc :
	$(MAKE) -C modreloc
endif


ifeq ($(BUILD), jellystone)
.PHONY: everything
everything : release_player release_libs
endif

ifeq ($(BUILD), kennedy)
.PHONY: everything
everything : release_player release_libs
endif

ifeq ($(BUILD), tardis)
.PHONY: everything
everything : release

.PHONY: release
release : release_makeupgrade release_romboot release_modules release_kernel downloadable-root downloadable-ramdisk-root

.PHONY: downloadable-root
downloadable-root : release_player release_modules release_init release_play release_libs

.PHONY: release_kernel
release_kernel : release_module_tools
	$(MAKE) -C kernel-2.4 zImage

.PHONY: release_init
release_init :
	$(MAKE) -C car-boot downloadable-root

.PHONY: release_kernel_image
release_kernel_image :
	@echo -n "Making kernel image... "
	@$(MAKE) -C kernel-2.4 zImage > kernel.log 2>&1 || ( echo "failed" ; cat kernel.log ; exit 1 )
	@echo done
endif # tardis

ifeq ($(BUILD), whitechapel)
.PHONY: everything
everything : release

.PHONY: release
release : release_makeupgrade release_romboot release_modules release_kernel downloadable-root downloadable-ramdisk-root

.PHONY: downloadable-root
downloadable-root : release_player release_modules release_init release_play release_libs

release_kernel : release_kernel_image

release_kernel_image : release_module_tools
	@echo -n "Making kernel image... "
	@$(MAKE) -C kernel-2.4 zImage > kernel.log 2>&1 || ( echo "failed" ; cat kernel.log ; exit 1 )
	@echo done
	mkdir -p $(DISK_ROOT)
	cp kernel-2.4/arch/arm/boot/zImage $(DISK_ROOT)zImage


#.PHONY: release_kernel
#release_kernel : release_module_tools
#	$(MAKE) -C kernel-2.4 zImage

.PHONY: release_init
release_init :
	$(MAKE) -C car-boot downloadable-root
endif # whitechapel

ifeq ($(BUILD), camelot)
.PHONY: everything
everything : release

.PHONY: release
release : release_makeupgrade release_romboot release_modules release_kernel downloadable-root downloadable-ramdisk-root

.PHONY: downloadable-root
downloadable-root : release_player release_modules release_init release_play release_libs

release_kernel : release_kernel_image

release_kernel_image : release_module_tools
	@echo -n "Making kernel image... "
	@$(MAKE) -C kernel-2.4 zImage > kernel.log 2>&1 || ( echo "failed" ; cat kernel.log ; exit 1 )
	@echo done
	mkdir -p $(DISK_ROOT)
	cp kernel-2.4/arch/arm/boot/zImage $(DISK_ROOT)zImage


#.PHONY: release_kernel
#release_kernel : release_module_tools
#	$(MAKE) -C kernel-2.4 zImage

.PHONY: release_init
release_init :
	$(MAKE) -C car-boot downloadable-root
endif # whitechapel

ifeq ($(BUILD), granite)
ifeq ($(ARCH),arm)
.PHONY: everything
everything : release release_libs

.PHONY: release
release : release_kernel downloadable-root release_romboot downloadable-ramdisk-root

.PHONY: downloadable-root
downloadable-root : release_player release_fonts release_init release_play release_libs

.PHONY: release_kernel
release_kernel :
	$(MAKE) -C kernel-2.4 zImage

.PHONY: release_init
release_init :
	$(MAKE) -C car-boot downloadable-root
else
ifeq ($(ARCH),armecos)
.PHONY: everything
everything: release

.PHONY: release
release:
	$(MAKE) -C lib/ecos rebuild
	$(MAKE) -C player strip
endif
endif
endif

ifeq ($(BUILD), onyx)
.PHONY: everything
everything : release release_libs

.PHONY: release
release : downloadable-root downloadable-ramdisk-root

.PHONY: downloadable-root
downloadable-root : release_player release_fonts release_libs

endif


ifeq ($(BUILD), jupiter)
.PHONY: everything
everything : release_binary

.PHONY: release
release : release_binary release_source

.PHONY: release_binary
release_binary : release_kernel downloadable-root release_romboot \
	downloadable-ramdisk-root release_upgrader

.PHONY: release_source
release_source : aux_source_release games_source_release \
	kernel_source_release distro_source_release modutils_source_release

.PHONY: downloadable-root
downloadable-root : release_player release_fonts release_init release_testcard release_play release_modules release_kernel release_aux release_games release_libs

ifeq ($(ARCH),pc)
release_kernel:
else
release_kernel : release_kernel_image
endif 

release_kernel_image : release_module_tools
	@echo -n "Making kernel image... "
	@$(MAKE) -C kernel-2.4 zImage > kernel.log 2>&1 || ( echo "failed" ; cat kernel.log ; exit 1 )
	@echo done
	mkdir -p $(DISK_ROOT)
	cp kernel-2.4/arch/arm/boot/zImage $(DISK_ROOT)zImage

release_init :
	$(MAKE) -C car-boot downloadable-root

release_testcard :
	$(MAKE) -C tests/jupiter downloadable-root

release_upgrader : release_kernel
	$(MAKE) -C pump

endif # jupiter


endif # ARCH != win32
