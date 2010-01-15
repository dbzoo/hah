/*
Contents:
   size_t getURL( void** data, char* url )
       Get the data at a URL and return it.
   size_t getURLbyParts( void** data, char* machine, int port, char* file )
       Same as getURL except that the URL is specified differently.

       For both of these, caller is responsible for calling 'free' on
       returned *data. Returns 0 if something wrong.

   int fd_printf
       printf to a file descriptor

   main() (Originally for testing)
       Get the contents of some command line URLs and put
       the contents to standard out.
       There are no error messages other than a usage message.

   PROBLEMS: 
       - No timeouts
       - Doesn't check all error returns
       - Only handles URLs that begin with http://
       - stdnet.c does far more than necessary
       - could use more testing

   See URL: http://info.cern.ch/hypertext/WWW/Protocols/HTTP/Request.html
   for information regarding the protocol.

 */


#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
 #include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ERROR 0
#define BUF_SIZE 4096
/* Charles 08/20/2003 */
#define STATUS_FILE "/var/ddns_status"
#define DDNS_URL "members.dyndns.org"

/* Globals */
//char file_path[64]="/var/fyi/wan/";



/*
 * Charles 08/19/2003
 */
 void stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *server;
	server=gethostbyname(hostname);
	if (server == NULL) {
		printf("DDNS--bad ddns server! server name=%s",hostname);
		exit(1);
	}
	if (server->h_length != 4) {
		printf("DDNS: oops %d\n",server->h_length);
		exit(1);
	}
	memcpy(&(p->s_addr),server->h_addr_list[0],4);
}


/* printf to a file descriptor */
int fd_printf(int fd, char *format, ...)
{
  va_list args;
  int fd2, len;
  FILE *fp;

  va_start(args, format);

  if (format == NULL)
    return 0;

  if (((fd2 = dup(fd))          == -1)   || /* Don't want to close the orig */
      ((fp  = fdopen(fd2, "w")) == NULL) || /* fd in the fclose below...    */
      ((len = vfprintf(fp, format, args)) == EOF) ||
      (fclose(fp) == EOF))
    {
      fprintf(stderr, "fd_printf can't print to %d using %s\n", fd, format);
      len = -1;
    }

  va_end(args);

  return len;
}


size_t getURLbyParts( void** data, char* machine, int port, char* username,  char* password,  char* hostname, char *myip)
{
    int sockfd;
    int bytes;
    char buf[BUF_SIZE],str_pid[6];
    char* results = 0;
    size_t size;
    int pid,time_count=0,status;
    char system_cmd[64];
    char user[256];
    char auth[512];
    struct sockaddr_in serv_addr;

    *data = 0;
    /* open a TCP socket */
    if ( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        printf("getURLbyParts():  can't open stream socket\n");
    /* Make the server's address structure that we want to connect */
     bzero( (char *) &serv_addr,sizeof(serv_addr) );
     serv_addr.sin_family =  AF_INET;
     stuff_net_addr(&(serv_addr.sin_addr),machine);
     serv_addr.sin_port = htons(port);
      /* connect to the server */
     if ( (pid=fork()) == -1 )
        {
         printf("getURLbyParts():fork error!\n");
         return 0;
        }
     else  if ( pid == 0 )	//child
        {
	        /* connect to the server */
	        if ( connect(sockfd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in)) <0 )
	                {
	                printf("DDNS: can't connect to server\n");
	                system("echo > /var/fw_reject");
	                exit(0);
	                }
	         system("echo > /var/fw_connect");
	         //printf("getURLbyParts():child exit with status\n");
	         exit(0);
	  }
 	else	//parent
     	{
          while ( open("/var/fw_connect",O_RDONLY) <= 0 )
                {
                 if  ( open("/var/fw_reject",O_RDONLY) > 0 )
                        {
                         unlink("/tmp/fw_reject");
                         wait(&status);
                         close(sockfd);
                         return 0;
                        }
                 if ( time_count >10 )
                       {
                        sprintf(str_pid,"%d",pid);
                        sprintf(system_cmd,"kill -9 %s",str_pid);
                        system(system_cmd);
                        wait(&status);
                        close(sockfd);
                        return 0;
                       }
                 sleep(1);
                 time_count++;
                 //printf("%d secs passed\n",time_count);
          	}
     	}
	
	wait(&status);
	unlink("/var/fw_connect");

	sprintf(user, "%s:%s", username, password);
	base64Encode(user, auth);
	//printf("user_name(%s), password(%s), user(%s), auth(%s)\n",username, password,user,auth);

	/* send GET message to http */
	if ( fd_printf( sockfd, "GET /nic/update?system=dyndns&hostname=%s&myip=%s&offline=NO HTTP/1.0\r\nAuthorization: Basic %s\r\nUser-Agent: myclient/1.0 me@null.net\r\nHost: %s\r\n\r\n",hostname,myip,auth,machine ) <= 0 ) {
	  printf("DDNS: HTTP GET request failed...\n");
	  close(sockfd);
	  return ERROR;
	}

	/* get data */
	size = 0;
	results = 0;

	while( (bytes = read(sockfd,buf,BUF_SIZE)) > 0) {
		  int newSize = size + bytes;
		  results = realloc( results, newSize );
		  assert( results );
		  bcopy( buf, results + size, bytes );
		  size += bytes;
	}
	*data = (void*)results;
	close(sockfd);

	return size;
}

/*
   Get the data at a URL and return it.
   Caller is responsible for calling 'free' on
   returned *data.
   url must be of the form http://machine-name[:port]/file-name
   Returns <0 if something wrong.
                -1 request to wrong server
                <-1 update error message
            0 if success
*/
size_t getURL( void** data, char* url , char* username,  char* password,  char* hostname,  char* myip)
{
  size_t size;
  int port=80;
  int status=0,retval=-1;
  char  ver[16]="";
  FILE *fd=fopen(STATUS_FILE, "w");
  if ( strcmp("members.dyndns.org",url) == 0 )
     {
      size = getURLbyParts( data, url, port, username,password,hostname,myip);
      if ( sscanf(*data, "%s %d",ver,&status) != 1)   
          {
                retval = -1;
          }
//printf("returned data: ver=%s, status=%d\n", ver, status);
      switch (status)
        {
        case -1:
                fputs("Error: Request to wrong server", fd);
                retval = -1;
                break;
        case 200:
                if(strstr(*data, "\ngood ") )
                        {
                        fputs("Update successful!", fd);
                        retval = 0;
                        }
                else
                        {
                        if(strstr(*data, "\nnohost") )
                                {
                                fputs("Failed: Invalid hostname", fd);
                                retval = -2;
                                }
                        else if(strstr(*data, "\nnotfqdn") )
                                {
                                fputs("Failed: Malformed hostname", fd );
                                retval = -3;
                                }
                        else if(strstr(*data, "\n!yours") )
                                {
                                fputs("Failed: Not your host!", fd);
                                retval = -4;
                                }
                        else if(strstr(*data, "\nabuse") )
                                {
                                fputs("Failed: Host has been blocked for abuse", fd);
                                retval = -5;
                                }
                        else if(strstr(*data, "\nnochg") )
                                {
                                fputs("Update successful! (But IP no change since last update)", fd);
                                // lets say that this counts as a successful update
                                retval = 0;
                                }
                        else if(strstr(*data, "\nbadauth") )
                                {
                                fputs("Failed: Authentication failure", fd);
                                retval = -6;
                                }
                        else if(strstr(*data, "\nbadsys") )
                                {
                                fputs("Failed: Invalid system parameter to server", fd);
                                retval = -7;
                                }
                        else if(strstr(*data, "\nbadagent") )
                                {
                                fputs("Failed: Bad user agent", fd);
                                retval = -8;
                                }
                        else if(strstr(*data, "\nnumhost") )
                                {
                                fputs("Failed: Too many or too few hosts found", fd);
                                retval = -9;
                                }
                        else if(strstr(*data, "\ndnserr") )
                                {
                                fputs("Failed: DynDNS internal error, please report this to DynDNS", fd);
                                retval = -10;
                                }
                        else if(strstr(*data, "\n911") )
                                {
                                 fputs("Failed: DynDNS internal error, please report this to DynDNS", fd);
                                retval = -11;
                                }
                        else if(strstr(*data, "\n!donator") )
                                {
                                fputs("Failed: Service only available to DynDNS donators", fd);
                                retval = -13;
                                }
                        else
                                {
                                fputs("Failed: Error processing request", fd);
                                retval = -1;
                                }
                       }
                break;
        case 401:
                fputs("Failed: Authentication failure", fd);
                retval = -14;
                break;
        default:
              fputs("Failed: Unknown return code", fd);
              retval = -1;
              break;
        }  // switch
     } 

  fclose(fd);
  return retval;
}

void main( int argc, char** argv )
{
  if (argc < 5) {
      printf("Usage: ddns username password hostname myip\n");
      exit( 1 );
    }
  //Charles 08/20/2003, for per PVC configuration
  //strcat(file_path, argv[5]);
  //strcat(file_path, "/");

  set_dns(DDNS_URL,argv[1],argv[2],argv[3],argv[4]); 
}

void set_dns(char *url,char *username,char *password,char *hostname,  char* myip)
{
  void* data;
  size_t ret_code;

    ret_code = getURL( &data, url ,username, password, hostname, myip);

    //printf("set_dns(): ret_code=%d\n",ret_code);
    //printf("returned HTTP content:\n%s\n",data);
}

static char table64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64Encode(char *intext, char *output)
{
  unsigned char ibuf[3];
  unsigned char obuf[4];
  int i;
  int inputparts;

  while(*intext) {
    for (i = inputparts = 0; i < 3; i++) {
      if(*intext) {
        inputparts++;
        ibuf[i] = *intext;
        intext++;
      }
      else
        ibuf[i] = 0;
    }

    obuf [0] = (ibuf [0] & 0xFC) >> 2;
    obuf [1] = ((ibuf [0] & 0x03) << 4) | ((ibuf [1] & 0xF0) >> 4);
    obuf [2] = ((ibuf [1] & 0x0F) << 2) | ((ibuf [2] & 0xC0) >> 6);
    obuf [3] = ibuf [2] & 0x3F;

    switch(inputparts) {
      case 1: /* only one byte read */
        sprintf(output, "%c%c==",
            table64[obuf[0]],
            table64[obuf[1]]);
        break;
      case 2: /* two bytes read */
        sprintf(output, "%c%c%c=",
            table64[obuf[0]],
            table64[obuf[1]],
            table64[obuf[2]]);
        break;
      default:
        sprintf(output, "%c%c%c%c",
            table64[obuf[0]],
            table64[obuf[1]],
            table64[obuf[2]],
            table64[obuf[3]] );
        break;
    }
    output += 4;
  }
  *output=0;
}

