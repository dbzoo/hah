/*
 * Copyright (c) 2d3D, Inc.
 * Written by Abraham vd Merwe <abraham@2d3d.co.za>
 * All rights reserved.
 *
 * $Id: f2fcp.c,v 1.1.1.1 2003/02/28 09:18:55 cmo Exp $
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/mtd/mtd.h>

typedef int bool;
#define true 1
#define false 0

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define DEBUG

/* for debugging purposes only */
#ifdef DEBUG
#undef DEBUG
#define DEBUG(fmt,args...) { log_printf (LOG_ERROR,"%d: ",__LINE__); log_printf (LOG_ERROR,fmt,## args); }
#else
#undef DEBUG
#define DEBUG(fmt,args...)
#endif

#define KB(x) ((x) / 1024)
#define PERCENTAGE(x,total) (((x) * 100) / (total))

/* size of read/write buffer */
#define BUFSIZE (8 * 1024)

/* cmd-line flags */
#define FLAG_NONE         0x00
#define FLAG_VERBOSE      0x01
#define FLAG_HELP         0x02
#define FLAG_DEVICE_SRC   0x04
#define FLAG_DEVICE_DEST  0x08

/* error levels */
#define LOG_NORMAL	1
#define LOG_ERROR	2

static void log_printf (int level,const char *fmt, ...)
{
   FILE *fp = level == LOG_NORMAL ? stdout : stderr;
   va_list ap;
   va_start (ap,fmt);
   vfprintf (fp,fmt,ap);
   va_end (ap);
   fflush (fp);
}

static void showusage (const char *progname,bool error)
{
   int level = error ? LOG_ERROR : LOG_NORMAL;

   log_printf (level,
			   "\n"
			   "Flash to Flash Copy - Written by Xavier DEBREUIL <xde@inventel.fr>\n"
			   "\n"
			   "usage: %s [ -v | --verbose ] <device_src> <device_dest>\n"
			   "       %s -h | --help\n"
			   "\n"
			   "   -h | --help      Show this help message\n"
			   "   -v | --verbose   Show progress reports\n"
			   "   <device_src>     Flash device to read from (e.g. /dev/mtd0, /dev/mtd1, etc.)\n"
			   "   <device_dest>    Flash device to write to (e.g. /dev/mtd0, /dev/mtd1, etc.)\n"
         "                    <device_src> and <device_dest> must be different\n"
			   "\n",
			   progname,progname);

   exit (error ? EXIT_FAILURE : EXIT_SUCCESS);
}

static int safe_open (const char *pathname,int flags)
{
   int fd;

   fd = open (pathname,flags);
   if (fd < 0)
	 {
		log_printf (LOG_ERROR,"While trying to open %s",pathname);
		if (flags & O_RDWR)
		  log_printf (LOG_ERROR," for read/write access");
		else if (flags & O_RDONLY)
		  log_printf (LOG_ERROR," for read access");
		else if (flags & O_WRONLY)
		  log_printf (LOG_ERROR," for write access");
		log_printf (LOG_ERROR,": %m\n");
		exit (EXIT_FAILURE);
	 }

   return (fd);
}

static void safe_read (int fd,const char *filename,void *buf,size_t count,bool verbose)
{
   ssize_t result;

   result = read (fd,buf,count);
   if (count != result)
	 {
		if (verbose) log_printf (LOG_NORMAL,"\n");
		if (result < 0)
		  {
			 log_printf (LOG_ERROR,"While reading data from %s: %m\n",filename);
			 exit (EXIT_FAILURE);
		  }
		log_printf (LOG_ERROR,"Short read count returned while reading from %s\n",filename);
		exit (EXIT_FAILURE);
	 }
}

static void safe_rewind (int fd,const char *filename)
{
   if (lseek (fd,0L,SEEK_SET) < 0)
	 {
		log_printf (LOG_ERROR,"While seeking to start of %s: %m\n",filename);
		exit (EXIT_FAILURE);
	 }
}

/******************************************************************************/

static int dev_src_fd = -1, dev_dest_fd = -1, fil_fd = -1 ;

static void cleanup (void)
{
   if (dev_src_fd > 0) close (dev_src_fd);
   if (dev_dest_fd > 0) close (dev_dest_fd);
   if (fil_fd > 0) close (fil_fd);
}

int main (int argc,char *argv[])
{
   const char *progname,*device_src = NULL,*device_dest = NULL;
   int i,flags = FLAG_NONE;
   size_t result,size,written;
   struct mtd_info_user mtd_src, mtd_dest ;
   struct erase_info_user erase_dest ;
   struct stat filestat;
   unsigned char src[BUFSIZE],dest[BUFSIZE];

   (progname = strrchr (argv[0],'/')) ? progname++ : (progname = argv[0]);

   /*************************************
	* parse cmd-line args back to front *
	*************************************/

   while (--argc)
	 {
		if (device_dest == NULL)
		  {
			 flags |= FLAG_DEVICE_DEST;
			 device_dest = argv[argc];
			 DEBUG("Got device to write to : %s\n", device_dest) ;
		  }
		else if (device_src == NULL)
		  {
			 flags |= FLAG_DEVICE_SRC;
			 device_src = argv[argc];
			 DEBUG("Got device to read from : %s\n", device_src) ;
		  }
		else if (!strcmp (argv[argc],"-v") || !strcmp (argv[argc],"--verbose"))
		  {
			 flags |= FLAG_VERBOSE;
			 DEBUG("Got FLAG_VERBOSE\n");
		  }
		else if (!strcmp (argv[argc],"-h") || !strcmp (argv[argc],"--help"))
		  {
			 flags |= FLAG_HELP;
			 DEBUG("Got FLAG_HELP\n");
		  }
		else
		  {
			 DEBUG("Unknown parameter: %s\n",argv[argc]);
			 showusage (progname,true);
		  }
	 }
   if (flags & FLAG_HELP || progname == NULL || device_src == NULL || device_dest == NULL)
	 showusage (progname,flags != FLAG_HELP);

   atexit (cleanup);

   /* get some info about the flash source device */
   dev_src_fd = safe_open(device_src,O_SYNC | O_RDWR) ;
   if (ioctl(dev_src_fd, MEMGETINFO, &mtd_src) < 0)
	 {
		DEBUG("ioctl(): %m\n");
		log_printf (LOG_ERROR,"This doesn't seem to be a valid MTD flash device!\n");
		exit (EXIT_FAILURE);
	 }

   /* get some info about the flash device */
   dev_dest_fd = safe_open (device_dest,O_SYNC | O_RDWR);
   if (ioctl (dev_dest_fd,MEMGETINFO,&mtd_dest) < 0)
	 {
		DEBUG("ioctl(): %m\n");
		log_printf (LOG_ERROR,"This doesn't seem to be a valid MTD flash device!\n");
		exit (EXIT_FAILURE);
	 }

   /* does the source device/partition fit into the destination device/partition? */
   if (mtd_src.size > mtd_dest.size)
	 {
		log_printf(LOG_ERROR, "%s won't fit into %s!\n", device_src, device_dest) ;
		exit(EXIT_FAILURE) ;
	 }

   /*****************************************************
	* erase enough blocks so that we can write the file *
	*****************************************************/

  for (erase_dest.start = 0 ; erase_dest.start < mtd_dest.size ; erase_dest.start += mtd_dest.erasesize)
  {
	   printf( "\rErasing %ld Kbyte @ %lx -- %2ld %% complete.", 
		   mtd_dest.erasesize / 1024, erase_dest.start, erase_dest.start * 100 / mtd_dest.size ) ;
     fflush( stdout );
	
    if(ioctl(dev_dest_fd, MEMERASE, &erase_dest) != 0)
      exit(EXIT_FAILURE) ;
   }


   /**********************************
	* write the entire file to flash *
	**********************************/

    if (flags & FLAG_VERBOSE)
      log_printf (LOG_NORMAL, "Writing data: 0k/%luk (0%%)",KB (mtd_src.size));
   size = mtd_src.size;
   i = BUFSIZE;
   written = 0;
   while (size)
	 {
		if (size < BUFSIZE)
      i = size;
		if (flags & FLAG_VERBOSE)
		  log_printf (LOG_NORMAL,"\rWriting data: %dk/%luk (%lu%%)",
				  KB (written + i),
				  KB (mtd_src.size),
				  PERCENTAGE (written + i, mtd_src.size));

		/* read from mtd_src */
		safe_read(dev_src_fd, device_src, src, i, flags & FLAG_VERBOSE);

		/* write to device */
		result = write(dev_dest_fd, src, i) ;
		if (i != result)
		  {
			 if (flags & FLAG_VERBOSE) log_printf (LOG_NORMAL,"\n");
			 if (result < 0)
			   {
				  log_printf (LOG_ERROR,
						   "While writing data to 0x%.8x-0x%.8x on %s: %m\n",
						   written,written + i,device_dest);
				  exit (EXIT_FAILURE);
			   }
			 log_printf (LOG_ERROR,
					  "Short write count returned while writing to x%.8x-0x%.8x on %s: %d/%lu bytes written to flash\n",
					  written,written + i,device_dest,written + result, mtd_src.size);
			 exit (EXIT_FAILURE);
		  }

		written += i;
		size -= i;
	 }

   if (flags & FLAG_VERBOSE)
	 log_printf (LOG_NORMAL,
				 "\rWriting data: %luk/%luk (100%%)\n",
				 KB (filestat.st_size),
				 KB (filestat.st_size));
   DEBUG("Wrote %d / %luk bytes\n",written,filestat.st_size);

   /**********************************
	* verify that flash == file data *
	**********************************/

   safe_rewind(dev_src_fd, device_src) ;
   safe_rewind(dev_dest_fd, device_dest) ;

   size = mtd_src.size;
   i = BUFSIZE;
   written = 0;
   if (flags & FLAG_VERBOSE)
    log_printf (LOG_NORMAL,"Verifying data: 0k/%luk (0%%)",KB (filestat.st_size));

   while (size)
	 {
		if (size < BUFSIZE) i = size;
		if (flags & FLAG_VERBOSE)
		  log_printf (LOG_NORMAL,
					  "\rVerifying data: %dk/%luk (%lu%%)",
					  KB (written + i),
					  KB (filestat.st_size),
					  PERCENTAGE (written + i,filestat.st_size));

		/* read from filename */
		safe_read(dev_src_fd, device_src, src, i, flags & FLAG_VERBOSE) ;

		/* read from device */
		safe_read(dev_dest_fd, device_dest, dest, i, flags & FLAG_VERBOSE) ;

		/* compare buffers */
		if (memcmp (src,dest,i))
		  {
			 log_printf (LOG_ERROR,
					  "File does not seem to match flash data. First mismatch at 0x%.8x-0x%.8x\n",
					  written,written + i);
			 exit (EXIT_FAILURE);
		  }

		written += i;
		size -= i;
	 }

   if (flags & FLAG_VERBOSE)
	 log_printf (LOG_NORMAL,
				 "\rVerifying data: %luk/%luk (100%%)\n",
				 KB (filestat.st_size),
				 KB (filestat.st_size));
   DEBUG("Verified %d / %luk bytes\n",written,filestat.st_size);

   exit (EXIT_SUCCESS);
}
