/* $Id: utils.c 43 2009-11-21 16:42:02Z brett $
 */
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h> 
#include <signal.h>

void upperstr(char *s) {
    while(*s) {
	*s = toupper(*s);
	s++;
    }
}

int subprocess(char **arg) {
    int fd, maxfd;
    signal(SIGCHLD, SIG_IGN); // Avoid creating zombies
    switch(fork()) {
    case 0: // child
	break;
    case -1: //fork failed
	return -1;
    default: // parent
	return 0;
    }

    // now running child
    if(setsid() < 0) return -1;
    switch(fork()) {
    case 0: break;
    case -1: return -1;
    default:
	_exit(0);
    }
    chdir("/");
    maxfd = sysconf(_SC_OPEN_MAX);
    while(fd < maxfd)
	close(fd++);
    open("/dev/null",O_RDWR);
    dup(0);
    dup(0);

    execv(arg[0], arg);
}
