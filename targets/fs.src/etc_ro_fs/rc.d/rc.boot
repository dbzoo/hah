#!/bin/sh

REDBOOT_LOADER_MTD=$(cat /proc/mtd | grep \"RedBoot\" | sed -n "s/\(mtd\)\(.\)*:\(.\)*/\1\2/p")

FORMAT_CREATE_JFFS2()
{
    echo "Restore - Format and Create JFFS2"
    umount /mnt/jffs2/jffs2_3

    [ -f /usr/sbin/grow ] &&  /usr/sbin/grow 0xA0000  /usr/sbin/fst

    if [ -n "$REDBOOT_LOADER_MTD" ] ; then
	/usr/sbin/eraseall /dev/mtd$(cat /proc/mtd | grep jffs_system | sed -n "s/\(mtd\)\(.*\):\(.\)*/\2/p")
	mount -t jffs2  /dev/mtdblock$(cat /proc/mtd | grep jffs_system | sed -n "s/\(mtd\)\(.*\):\(.\)*/\2/p") /mnt/jffs2/jffs2_3
    else
	/usr/sbin/eraseall /dev/mtd3
	mount -t jffs2  /dev/mtdblock3 /mnt/jffs2/jffs2_3
    fi
    
    POPULATE_JFFS2
}


POPULATE_JFFS2()
{
    echo "Restore - Populate JFFS2"
    # PUT FACTORY PARAMETERS IN JFFS2
    mkdir /mnt/jffs2/jffs2_3/etc
    for i in inetd.conf issue.net fstab build group hosts passwd resolv.conf xap.d/; do
      /bin/cp -a /etc_ro_fs/$i /mnt/jffs2/jffs2_3/etc
    done
    mkdir /mnt/jffs2/jffs2_3/etc/plugboard
    mkdir /mnt/jffs2/jffs2_3/tmp
    mkdir /mnt/jffs2/jffs2_3/home
    mkdir /mnt/jffs2/jffs2_3/root

    for i in init.d rc1.d rc.d; do
      mkdir /mnt/jffs2/jffs2_3/etc/$i
      ln -sf /etc_ro_fs/$i/* /mnt/jffs2/jffs2_3/etc/$i/
    done

    # Links to RO filesystem
    for i in inittab shells services ethertypes curl-ca-bundle.crt; do
      ln -sf /etc_ro_fs/$i /etc/$i
    done

    sleep 1
    touch /mnt/jffs2/jffs2_3/etc/finished
}


# BEGIN OF MAIN PROCESS

# DEBUG SCRIPT
#set -x

if [ ! -e /etc/rc.d ]
then
    /bin/rm -f /tmp/touch.test

    if [ -n "$REDBOOT_LOADER_MTD" ] ; then
       	    #cat /proc/mtd
	    /bin/mount -t jffs2 -o rw /dev/mtdblock$(cat /proc/mtd | grep jffs_system | sed -n "s/\(mtd\)\(.*\):\(.\)*/\2/p") /mnt/jffs2/jffs2_3
	else
	    /bin/mount -t jffs2 -o rw /dev/mtdblock3 /mnt/jffs2/jffs2_3
	fi

    # test if jffs2 is working as expected (rw)
    /bin/rm -f /tmp/touch.test
    /bin/touch /tmp/touch.test
    if [ ! -e /tmp/touch.test ]
    then
	# JFFS2 was not correctly mounted.
	# START RECOVERY
	FORMAT_CREATE_JFFS2
	# test if jffs2 is working as expected (rw)
	/bin/rm -f /tmp/touch.test
	/bin/touch /tmp/touch.test
	if [ ! -e /tmp/touch.test ]
	then
	    # JFFS2 was not correctly mounted.
	    # START RECOVERY
	    FORMAT_CREATE_JFFS2
	    /sbin/reboot
	else
	    /bin/rm -f /tmp/touch.test
	    if [ -e /tmp/touch.test ]
	    then
	    # JFFS2 was not correctly mounted : unable to erase a file in it.
		FORMAT_CREATE_JFFS2
		/sbin/reboot
	    fi
	fi  
#	/sbin/reboot
    else
	/bin/rm -f /tmp/touch.test
	if [ -e /tmp/touch.test ]
	then
          # JFFS2 was not correctly mounted : unable to erase a file in it.
	  FORMAT_CREATE_JFFS2
	  /sbin/reboot
	fi
    fi

    if [ ! -f /mnt/jffs2/jffs2_3/etc/issue.net ]
    then # should happen only when FACTORY SETTINGS RECOVERY has happened
	FORMAT_CREATE_JFFS2
  	/sbin/reboot
    fi

    if [ ! -f /mnt/jffs2/jffs2_3/etc/finished ]
    then
        FORMAT_CREATE_JFFS2
	/sbin/reboot
    fi

fi

exit 0
