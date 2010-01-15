CFLAGS = -Wall -Os
ifeq ($(strip $(BUILD_BRCTL)),static)
CFLAGS += -DBUILD_STATIC
endif
all:			brctl brctld

dynamic:		brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			$(CC) -Wl,--allow-shlib-undefined -Wall -s -o brctl brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a

static:			brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			cp ../libbridge/libbridge.a ../brctl.a
			$(AR) rcs ../brctl.a brctl.o brctl_cmd.o brctl_disp.o
			
			

brctl:			brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			cp ../libbridge/libbridge.a ../brctl.a
			#$(AR) rcs ../brctl.a brctl.o brctl_cmd.o brctl_disp.o
#			#$(RANLIB) brctl.a
			$(CC) -Wl,--allow-shlib-undefined -Wall -s -o brctl brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
#			$(CC) -Wl,--allow-shlib-undefined -static -Wall -o brctl brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a

brctld:			brctld.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			$(CC) -Wl,--allow-shlib-undefined -Wall -o brctld brctld.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a

clean:
			rm -f *.o
			rm -f brctl
			rm -f brctld
			rm -f core

brctl.o:		brctl.c brctl.h
			$(CC) -I../libbridge $(CFLAGS) -c -o brctl.o brctl.c

brctld.o:		brctld.c brctl.h
			$(CC) -I../libbridge $(CFLAGS) -c -o brctld.o brctld.c

brctl_cmd.o:		brctl_cmd.c brctl.h
			$(CC) -I../libbridge $(CFLAGS) -c -o brctl_cmd.o brctl_cmd.c

brctl_disp.o:		brctl_disp.c brctl.h
			$(CC) -I../libbridge $(CFLAGS) -c -o brctl_disp.o brctl_disp.c
