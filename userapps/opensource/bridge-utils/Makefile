all:				brctl/brctl brctl/brctld

install: 			 brctl/brctl.c brctl/brctl.h brctl/brctl_cmd.c brctl/brctl_disp.c libbridge/libbridge.a libbridge/libbridge.h
				make -C brctl dynamic
				install -m 755 brctl/brctl $(INSTALL_DIR)/bin
				$(STRIP) $(INSTALL_DIR)/bin/brctl

dynamic: 			install

static:  			brctl/brctl.c brctl/brctl.h brctl/brctl_cmd.c brctl/brctl_disp.c libbridge/libbridge.a libbridge/libbridge.h
				make -C brctl static

clean:
				make -C brctl clean
				make -C doc clean
				make -C libbridge clean
				make -C misc clean
				rcsclean *
				rm -f *~ *.a

brctl/brctl:			brctl/brctl.c brctl/brctl.h brctl/brctl_cmd.c brctl/brctl_disp.c libbridge/libbridge.a libbridge/libbridge.h
				make -C brctl

brctl/brctld:			brctl/brctld.c brctl/brctl.h brctl/brctl_cmd.c brctl/brctl_disp.c libbridge/libbridge.a libbridge/libbridge.h
				make -C brctl

libbridge/libbridge.a:		libbridge/if_index.c libbridge/libbridge.h libbridge/libbridge_compat.c libbridge/libbridge_devif.c libbridge/libbridge_if.c libbridge/libbridge_init.c libbridge/libbridge_misc.c libbridge/libbridge_private.h
				make -C libbridge

misc/bidi:			misc/bidi.c
				make -C misc

misc/tap:			misc/tap.c
				make -C misc
