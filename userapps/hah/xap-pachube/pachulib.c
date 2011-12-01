/***********************************************************************
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
( at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
ERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
Online: http://www.gnu.org/licenses/gpl.txt
***********************************************************************/

#include "pachulib.h"
#include "log.h"

/*PRIVATE FUNCTIONS*/
//socket related
int resolve_host(char *host,char *ip);
int connect_server(int *fd);
void disconnect_server(int fd);
int read_data(int fd, char *buffer);
int send_data(int fd, char *buffer, unsigned int long_buffer);

//http status codes:
int recover_status(char *msg);
int recover_env_id_status(char *msg,unsigned int * env_id);
int recover_trigger_id_status(char * msg,unsigned int * tr_id);
int recover_env_from_http(char *msg, char *environment);
int find_in_http(char *msg, char *tag, int * position);

int data_format_str(data_format_tp format, char * format_str);


int save_file(char * file_name, char * datastream);


char * http_method_tp_str[] = {"POST","GET","PUT","DELETE",NULL};

char * data_format_tp_str[] = {"csv","json","xml","png","rss","atom",NULL};

char * trigger_tp_str[] ={"gt","gte","lt","lte","eq","change", NULL};



int data_format_str(data_format_tp format, char * format_str)
{
	if ((format<CSV)||(format>ATOM)) return FALSE;

	if (format == CSV)
		sprintf(format_str,"csv");
	else if (format == JSON)
		sprintf(format_str,"json");
	else if (format == XML)
		sprintf(format_str,"xml");
	else if (format == PNG)
		sprintf(format_str,"png");
	else if (format == RSS)
		sprintf(format_str,"rss");
	else if (format == ATOM)
		sprintf(format_str,"atom");

	return TRUE;

}

int save_file(char * file_name, char * datastream)
{

	FILE *ofp;
	ofp = fopen(file_name, "w+"); //create if not exists

	if (ofp == NULL) 
	{
	  err_strerror("Can't open output file %s!\n");
	  return FALSE;
	}

	fprintf(ofp,"Data:\n\n%s\n\nEnd.",datastream);

	fflush(ofp);

	fclose(ofp);

	return TRUE;


}


int resolve_host(char * host, char *ip)
{
	struct hostent *he;

	if ((he=gethostbyname(host)) == NULL)   
        {
        	err_strerror("Err!! gethostbyname");
	        return FALSE;
    	}

	if(inet_ntop(AF_INET, (void *)he->h_addr_list[0], ip, IP_MAX) == NULL) //16 = xxx.xxx.xxx.xxx + '\0' (ip)
	{
	    	err_strerror("Err!! resolving host");
		return FALSE;
	}

	info("host vale %s, ip %s\n",host, ip);
  	return TRUE;
}
//fd: socket descriptor
int connect_server(int *fd)
{
    	struct sockaddr_in direccion;
	char ip[IP_MAX]; //xxx.xxx.xxx.xxx. + '\0'


	if (!resolve_host(HOST,ip))
		return FALSE;

	if ((*fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) 
	{
		err_strerror("Err!! socket");
		return FALSE;
	}
    	
	direccion.sin_family = AF_INET;    
	direccion.sin_port = htons(HTTP_PORT); 
	direccion.sin_addr.s_addr = inet_addr(ip);

    	memset(&(direccion.sin_zero), 8,0);  
    	if (connect(*fd, (struct sockaddr *)&direccion, sizeof(struct sockaddr)) == -1) 
	{
        	err_strerror("Err!! connect");
	        return FALSE;
        }
	info("connected\n");
	return TRUE;
}
void disconnect_server(int fd)
{
	close(fd);
	info("disconnected\n");
}
int send_data(int fd, char *buffer, unsigned int long_buffer)
{
	int ret = 0;


	info("\nSEND_DATA:\n%s\n",buffer);

	while (long_buffer)	
	{
		ret=send(fd, buffer, long_buffer, MSG_NOSIGNAL);

		if (ret == -1)
		{
			err_strerror("Err! send");
			close(fd);
			return FALSE;
		}
		long_buffer-=ret;
		buffer+=ret;
	}
	return TRUE;
}
int read_data(int fd, char *buffer)
{

	int long_buffer = MSG_MAX;
	int ret = 0;
	
	char * ptoB = buffer;

	while (long_buffer>0)
	{
		ret=recv(fd, ptoB,long_buffer, MSG_NOSIGNAL);
		//printf("\nret %d, long %d\n",ret,long_buffer);
		//printf("recibido %s\n",buffer);
		if (ret<0)
		{
			err_strerror("Err! recv");	
			close(fd);
			return FALSE;
		}
		if (ret == 0)
		{
			
			close(fd);
			if (long_buffer == MSG_MAX) //no-data
			{
				err_strerror("Err!! desconexion en recv");			
				return FALSE;
			}
			*ptoB='\0';
			return TRUE;
			
		}
		long_buffer-=ret;
		ptoB+=ret;
		
	}
	return TRUE;
}
int recover_status(char *msg)
{

	
	//Server return codes:
	int return_code;

	info("\n__READ_DATA:\n%s\nEND_DATA__\n",msg);

	if (sscanf(msg,"HTTP/1.1 %d*",&return_code) != 1) return FALSE;

	debug("HTTP return code : %d\n", return_code);

	if (return_code == RET_OK)
	{
		//printf("OK\n");	
		return TRUE;
	}
/* TODO 
#define RET_NOT_AUTH 401
#define RET_FORBIDDEN 403
#define RET_NOT_FOUND 404
#define RET_UNPROCESSABLE_ENTITY 422
#define RET_INTERNAL_SERVER_ERROR 500
#define RET_NO_SERVER_ERROR 503
*/
	return FALSE;
}
int recover_env_id_status(char *msg,unsigned int * env_id)
{
	if (msg == NULL) return FALSE; //strtok
	
	int nLength = strlen(msg);
        if (msg[nLength-1] == '\n') {
                msg[nLength-1] = '\0';
        }
        int found = FALSE;
        char * tok = strtok(msg, "\n");
        while (tok != NULL) {
                if (sscanf(tok,"Location: http://www.pachube.com/api/feeds/%d",env_id)==1)
                {
                        found = TRUE;
                        break;
                }
                tok = strtok(NULL,"\n");
        }
        if (found) notice("Environment id %d created\n",*env_id);
        

	return found;

}
int recover_trigger_id_status(char * msg,unsigned int * tr_id)
{
	if (msg == NULL) return FALSE; //strtok


	info("msg = %s\n",msg);
	
	int nLength = strlen(msg);
        if (msg[nLength-1] == '\n') {
                msg[nLength-1] = '\0';
        }
        int found = FALSE;
        char * tok = strtok(msg, "\n");
        while (tok != NULL) {
                if (sscanf(tok,"Location: http://www.pachube.com/api/triggers/%d",tr_id)==1)
                {
                        found = TRUE;
                        break;
                }
                tok = strtok(NULL,"\n");
        }
        if (found) notice("Trigger id %d created\n",*tr_id);
        

	return found;

}
int find_in_http(char *msg, char *tag, int * position)
{
	int i = 0;
	int j = 0;
	int tagL = strlen(tag);

	if ((msg == NULL) || (tag ==NULL)) return FALSE;


	while (msg[i]!='\0')
	{
		if (msg[i] != tag [j])
			j = 0;
		else
		{
			j++;
			if (j==tagL) //found
			{
				* position = i - tagL + 1;
				return TRUE;
			}
		}
		i++;
	}
	return FALSE;
}
int recover_env_from_http(char *msg, char *environment)
{

	if (msg == NULL) return FALSE;

	int offset =0;
	
	if (!find_in_http(msg,"<?xml",&offset)) return FALSE;

	//found:
	sprintf(environment,"%s",msg+offset);
	return TRUE;


}


/*PUBLIC FUNCTIONS*/

/*Realtime data, AUTH*/
//environmet related:

/**
* create a new environment: if eeml data is NULL it creates a default one.
**/
int create_environment(char *api_key, char *environment, unsigned int *env_id) //POST
{

	char msg[MSG_MAX];
	int fd;

	if (environment == NULL) //default environment: default_title
	{
		sprintf(environment,"<eeml xmlns=\"http://www.eeml.org/xsd/005\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.eeml.org/xsd/005 http://www.eeml.org/xsd/005/005.xsd\"><environment><title>%s</title></environment></eeml>",DEFAULT_ENV);
	}

	sprintf(msg,"POST /api/feeds/ HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",HOST, api_key,strlen(environment));
	//add body to msg:
	sprintf(msg,"%s%s",msg,environment);


	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_env_id_status(msg,env_id)) return FALSE;
	
	disconnect_server(fd);

	
	return TRUE;

}
/**
* delete an env_id environment
*/
int delete_environment(unsigned int env_id,char * api_key) //DELETE
{

	char msg[MSG_MAX];
	int fd = 0;

	//add header to msg
	sprintf(msg,"DELETE /api/feeds/%d HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",env_id, HOST, api_key);


	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;
	
	if (!recover_status(msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;

}

/**
* get xml env_id environment information
* returns xml body as string
*/
int get_environment(unsigned int env_id, char * api_key, char *environment,data_format_tp format) //GET
{

	char msg[MSG_MAX];
	int fd = 0;

	

	char format_str [STR_LEN];	
	if (!data_format_str(format,format_str)) return FALSE;



	//add header to msg:
	//TODO : xml now, more if needed in future..
	sprintf(msg,"GET /api/feeds/%d.%s HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",env_id,format_str,HOST, api_key,0);

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE; //TODO +'\0'..

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;

	if (format==XML)
	{
		if (!recover_env_from_http(msg,environment)) return FALSE;
	}else{
		//TODO
		sprintf(environment,"%s",msg);
	}
	
	//printf("\nREAD_DATA:\n%s\n",environment);

	disconnect_server(fd);
	
	return TRUE;

}
/*
* update enviroment: datastreams or info
*/
int update_environment(unsigned int env_id, char *api_key, char *environment,data_format_tp format) //PUT
{
	char msg[MSG_MAX];
	int fd = 0;
	
	

	char * format_str = data_format_tp_str[format]; 

//printf("formato dice: %s , tipo: %d\n",format_str,format);

	if ((format=!XML)&&(format!=CSV)) return FALSE; 

	//add header to msg:
	//TODO : xml now, more if needed in future..
	sprintf(msg,"PUT /api/feeds/%d.%s HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",env_id,format_str,HOST, api_key,strlen(environment));
	//add body to msg:
	sprintf(msg,"%s%s",msg,environment);

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;

}

//datastream functions

//Creates a new datastream in environment [environment ID]. 
//The body of the request should contain EEML of the environment to be created 
//and at least one datastream. 
int create_datastream(unsigned int env_id, char *api_key, char *environment) //POST
{

	char msg[MSG_MAX];
	int fd;

	//add header to msg:
	sprintf(msg,"POST /api/feeds/%d/datastreams/ HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",env_id, HOST, api_key,strlen(environment) );
	//add body to msg:
	sprintf(msg,"%s%s",msg,environment);

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;
	
	disconnect_server(fd);

	
	return TRUE;

}
int delete_datastream(unsigned int env_id, int ds_id, char *api_key) //DELETE
{

	char msg[MSG_MAX];
	int fd;

	//add header to msg:
	sprintf(msg,"DELETE /api/feeds/%d/datastreams/%d HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",env_id,ds_id, HOST, api_key, 0);
	//body no needed

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;
	
	disconnect_server(fd);

	
	return TRUE;

}
int get_datastream(unsigned int env_id,int ds_id,char *api_key, char *datastream, data_format_tp format) //GET
{

	char msg[MSG_MAX];
	int fd;


	char format_str [STR_LEN];	
	if (!data_format_str(format,format_str)) return FALSE;

	//TODO implement other data format while recovering
	sprintf(msg,"GET /api/feeds/%d/datastreams/%d.xml HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",env_id,ds_id,HOST,api_key);
//	sprintf(msg,"GET /api/feeds/%d/datastreams/%d.%s HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",env_id,ds_id,format_str, HOST,api_key);
		
	//no body this time

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;
	
	if (!recover_env_from_http(msg,datastream))
		return FALSE;

	/*
	if (format==XML)
	{
		if (!recover_env_from_http(msg,datastream)) return FALSE;
	}else{
		//TODO
		sprintf(datastream,"%s",msg);
	}
*/

	disconnect_server(fd);
	
	return TRUE;



}
int update_datastream(unsigned int env_id,int ds_id, char *api_key, char *datastream,data_format_tp format) //PUT
{
	char msg[MSG_MAX];
	int fd;
	
	//header:
	sprintf(msg,"PUT /api/feeds/%d/datastreams/%d.csv HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",env_id,ds_id,HOST, api_key,strlen(datastream));
	//body:
	sprintf(msg,"%s%s\r\n",msg,datastream);
	

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;

}


//triggers functions:
int create_trigger(trigger_tp * trigger,char *api_key) //POST
{

	char msg[MSG_MAX];
	char body[MSG_MAX];
	int fd;

	char type_str[STR_LEN];
 	sprintf(type_str,"%s",trigger_tp_str[trigger->type]);

	info("tipo trigger vale %d\n",trigger->type);


	//this time, body 1
	//important: POST meth doesnt allow use \r\n 
	sprintf(body,"trigger[url]=%s&trigger[trigger_type]=%s&trigger[threshold_value]=%d&trigger[environment_id]=%d&trigger[stream_id]=%d",trigger->url,type_str,trigger->threshold,trigger->env_id,trigger->ds_id);


	int post_l = strlen(body);

//	printf("body %s\n",body);

	//add header to msg:
	sprintf(msg,"POST /api/triggers/ HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",HOST, api_key, post_l, body);

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_trigger_id_status(msg,&(trigger->tr_id))) return FALSE;
	
	disconnect_server(fd);

	return TRUE; //created, tr_id returned
}


//TODO i dunno what to do with format??? header info unknown!
int get_all_triggers(data_format_tp format, char *api_key, char *trigger_list) //GET
{

	char msg[MSG_MAX];
	int fd;

	//char format_str [STR_LEN];	
	
	//if (!data_format_str(format,format_str)) return FALSE;
	
	//TODO resolve this!!!
	format = XML;
	
	
	//without the xml body
	sprintf(msg,"GET /api/triggers/ HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",HOST, api_key, 0);


	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE; //TODO +'\0'..

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;
	
	//TODO : corrected bug, need resolve all
	if (format==XML)
	{
		//printf("___________________________________________llega\n");
		if (!recover_env_from_http(msg,trigger_list)) return FALSE;
	}else{
		//TODO
		sprintf(trigger_list,"%s",msg);
	}
	
	disconnect_server(fd);
	
	return TRUE;



}


int update_trigger(trigger_tp *trigger, char *api_key)//PUT
{

	char msg[MSG_MAX];
	char body[MSG_MAX];
	int fd;

	char *type_str = trigger_tp_str[trigger->type];


	//this time, body 1
	//like post, 501 server error
	sprintf(body,"trigger[url]=%s&trigger[trigger_type]=%s&trigger[threshold_value]=%d&trigger[environment_id]=%d&trigger[stream_id]=%d",trigger->url,type_str,trigger->threshold,trigger->env_id,trigger->ds_id);
	//like xml:

/*<?xml version="1.0" encoding="UTF-8"?>
<datastream-trigger>
  <id type="integer">82</id>
  <url>http://www.postbin.org/zq17vl</url>
  <trigger-type>change</trigger-type>
  <threshold-value type="float">5</threshold-value>
  <notified-at type="datetime">2009-12-19T17:09:46Z</notified-at>
  <user>min0n</user>
  <environment-id type="integer">3975</environment-id>
  <stream-id>0</stream-id>
</datastream-trigger>*/

sprintf(body,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<datastream-trigger>\r\n<id type=\"integer\">%d</id>\r\n<url>%s</url>\r\n<trigger-type>%s</trigger-type><threshold-value type=\"float\">%d</threshold-value>\r\n<environment-id type=\"integer\">%d</environment-id>\r\n<stream-id>%d</stream-id>\r\n</datastream-trigger>",trigger->tr_id,trigger->url,type_str,trigger->threshold,trigger->env_id,trigger->ds_id);



	debug("body value %s\n",body);

	int post_l = strlen(body);

	//add header to msg:
	sprintf(msg,"PUT /api/triggers/%d HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",trigger->tr_id,HOST, api_key, post_l, body);




	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_trigger_id_status(msg,&(trigger->tr_id))) return FALSE;
	
	disconnect_server(fd);

	return TRUE; //created, tr_id returned

}

int get_trigger(int trigger_id, data_format_tp format, char *api_key, char *trigger)//GET
{

	char msg[MSG_MAX];
	int fd;


	if ((format=!XML)&&(format!=JSON)) return FALSE;

	sprintf(msg,"GET /api/triggers/%d HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",trigger_id, HOST, api_key, 0);


	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE; //TODO +'\0'..

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;

	
	if (format==XML)
	{
		if (!recover_env_from_http(msg,trigger)) return FALSE;
	}else{
		//TODO
		sprintf(trigger,"%s",msg);
	}
	
	//printf("\nREAD_DATA:\n%s\n",environment);

	disconnect_server(fd);
	
	return TRUE;


}

int delete_trigger(int trigger_id, char *api_key) //DELETE
{


	char msg[MSG_MAX];
	int fd = 0;

	//add header to msg
	sprintf(msg,"DELETE /api/triggers/%d HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",trigger_id, HOST, api_key);


	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;
	
	if (!recover_status(msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;

}
/*Historical data, NO_AUTH*/
int get_historical_datastream_csv(int env_id,int ds_id, char *file_name) //GET
{


	char msg[MSG_MAX];
	int fd;
//	char datastream[MSG_MAX];


	
	sprintf(msg,"GET /feeds/%d/datastreams/%d/history.csv HTTP/1.1\r\nHost: %s\r\n\r\n",env_id,ds_id,HOST);
//	sprintf(msg,"GET /api/feeds/%d/datastreams/%d.%s HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",env_id,ds_id,format_str, HOST,api_key);
		
	//no body this time

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;

	if (!recover_status(msg)) return FALSE;
	
	//if (!recover_env_from_http(msg,datastream))
	//	return FALSE;

	//TODO delete header
	if (!save_file(file_name, msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;




}
/*
int get_historical_datastream_png(int env_id,int ds_id, graph_tp graph,char *file_name) //GET
{

}
*/
int get_archive_datastream_csv(int env_id, int ds_id, char *file_name) //GET
{
char msg[MSG_MAX];
	int fd;
	//char datastream[MSG_MAX];


	
	sprintf(msg,"GET /feeds/%d/datastreams/%d/archive.csv HTTP/1.1\r\nHost: %s\r\n\r\n",env_id,ds_id,HOST);
//	sprintf(msg,"GET /api/feeds/%d/datastreams/%d.%s HTTP/1.1\r\nHost: %s\r\nX-PachubeApiKey: %s\r\n\r\n",env_id,ds_id,format_str, HOST,api_key);
		
	//no body this time

	if (!connect_server(&fd)) return FALSE;

	if (!send_data(fd,msg,strlen(msg)+1)) return FALSE;

	if (!read_data(fd, msg)) return FALSE;


	if (!recover_status(msg)) return FALSE;
	
	//if (!recover_env_from_http(msg,datastream))
	//	return FALSE;

	if (!save_file(file_name, msg)) return FALSE;

	disconnect_server(fd);
	
	return TRUE;

}




