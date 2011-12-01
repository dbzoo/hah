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

/*
*	pachulib: c pachube client library 
*	by min0n at 16 - Dic - 2009 / www.babuino.org
*
*	feel free to play with this code !
*       4D4 L0v3 >> (^_^).
*/


#ifndef PACHULIB_H
#define PACHULIB_H
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>     // signal
#include <sys/types.h>  // socket, connect,bind, sccept, send, recv, setsockopt    
#include <sys/socket.h> // socket, inet_addr, inet_ntoa, connect, bind, listen, accept, send, recv, setsockopt     
#include <netinet/in.h> // htons, inet_addr, inet_ntoa 
#include <arpa/inet.h>  // inet_addr, inet_ntoa
#include <unistd.h>     // close    
#include <string.h>     // memset, strerror,      
#include <errno.h>
#include <netdb.h>      // gethostbyname, gethostbyaddr,  

#define TRUE 1
#define FALSE 0


//IP_MAX = xxx.xxx.xxx.xxx + '\0'
#define IP_MAX 16
#define HTTP_PORT 80
#define HTTP  "http://"
#define HOST  "www.pachube.com"
#define API_KEY "pachube-api-key"
#define MSG_MAX 1500 //TODO: look at it
#define STR_LEN 200
#define KEY_LEN 200 //TODO: look at it


#define DEFAULT_ENV "as_you_like"

//Server return codes:
#define RET_OK 200
#define RET_NOT_AUTH 401
#define RET_FORBIDDEN 403
#define RET_NOT_FOUND 404
#define RET_UNPROCESSABLE_ENTITY 422
#define RET_INTERNAL_SERVER_ERROR 500
#define RET_NO_SERVER_ERROR 503

#define RET_ENV_CREATED 201

//http method types
typedef enum 
{
    POST,
    GET,
    PUT,
    DELETE
} http_method_tp;


//data format types
typedef enum
{
    CSV,
    JSON,
    XML,
    PNG,
    RSS,
    ATOM
} data_format_tp;


//trigger operation types
typedef enum
{
	GT,
	GTE,
	LT,
	LTE,
	EQ,
	CHANGE
}trigger_op_tp;


//graph configuration:
typedef struct
{
    unsigned int weigth;
    unsigned int height;
    char *color;
    char *title;
    char *legend;
    unsigned int stroke_size;
    unsigned int axix_labels;
    unsigned int detailed_grid;
} graph_tp;

typedef struct
{

	unsigned int env_id;
	unsigned int ds_id;
	unsigned int threshold;
	trigger_op_tp type;
	char url[STR_LEN];
	unsigned int tr_id;
} trigger_tp;

/*Realtime data, AUTH*/
//environmet related:
int create_environment(char *api_key, char *environment, unsigned int *env_id); //POST
int delete_environment(unsigned int env_id,char * api_key); //DELETE
int get_environment(unsigned int env_id, char * api_key, char *environment,data_format_tp format); //GET
int update_environment(unsigned int env_id, char *api_key, char *environment,data_format_tp format); //PUT

//datastream functions
int create_datastream(unsigned int env_id, char *api_key,char *environment); //POST
int delete_datastream(unsigned int env_id, int ds_id, char *api_key); //DELETE
int get_datastream(unsigned int env_id,int ds_id,char *api_key, char *datastream,data_format_tp format); //GET
int update_datastream(unsigned int env_id,int ds_id, char *api_key, char *datastream,data_format_tp format); //PUT

//triggers functions:
int create_trigger(trigger_tp * trigger, char *api_key); //POST
//int create_trigger(unsigned int env_id, unsigned int ds_id, unsigned int threshold, trigger_tp type, char * url,char *api_key,unsigned int *tr_id); //POST
int get_all_triggers(data_format_tp format, char *api_key, char *trigger_list); //GET
int update_trigger(trigger_tp * trigger, char *api_key);//PUT
int get_trigger(int trigger_id, data_format_tp format, char *api_key, char *trigger); //GET
int delete_trigger(int trigger_id, char *api_key); //DELETE

/*Historical data, NO_AUTH*/
int get_historical_datastream_csv(int env_id,int ds_id, char *file_name); //GET
int get_historical_datastream_png(int env_id,int ds_id, graph_tp graph,char *file_name); //GET
int get_archive_datastream_csv(int env_id, int ds_id, char *file_name); //GET

/*Collections: multiple feeds*/
//NO_AUTH:
int get_all_feed(data_format_tp format, char *file_name); //GET
int get_feed_by_tag(data_format_tp format, char *tag, char *file_name); //GET











#endif
