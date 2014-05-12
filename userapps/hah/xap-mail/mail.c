/**
 $Id$

 Requires: libcurl 7.20.0 or above.

 It allows you to send email:

 xap-header
 {
 uid=FF00DF00
 source=dbzoo.livebox.test
 target=dbzoo.wombat.Mail
 hop=1
 class=email
 v=12
 }
 message
 {
 to=brett@dbzoo.com
 subject=Email test from xAP
 text=Hello wombat
 }
 
 Brett England
 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.  

 Portions from smtp-tls.c and sendmail.c
 Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 Copyright (C) 2011, Tuan Ha, <tuan@tuanht.net>.
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "xap.h"
#include "minIni.h"
#include "log.h"

char *inifile = "/etc/xap.d/xap-mail.ini";
char smtp_server[64];
char smtp_username[64];
char fullname[64];
char *smtp_password=NULL;
char *interfaceName = NULL;
char *payload_text[6];

struct upload_status {
  int lines_read;
};

char* strdup_vprintf(const char* format, va_list ap)
{
  va_list ap2;
  int size;
  char* buffer;
 
  va_copy(ap2, ap);
  size = vsnprintf(NULL, 0, format, ap2)+1;
  va_end(ap2);
 
  buffer = malloc(size+1);
  assert(buffer != NULL);
 
  vsnprintf(buffer, size, format, ap);
  return buffer;
}
 
char* strdup_printf(const char* format, ...)
{
  char* buffer;
  va_list ap;
  va_start(ap, format);
  buffer = strdup_vprintf(format, ap);
  va_end(ap);
  return buffer;
}
 
size_t payload_source (void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;
 
  if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
 
  data = payload_text[upload_ctx->lines_read];
 
  if (data) {
    size_t len = strlen (data);
    memcpy (ptr, data, len);
    upload_ctx->lines_read ++;
    return len;
  }
  return 0;
}
 
int sendmail (const char *to,
              const char *subject,
              const char *body) 
{          
  CURL *curl;
  CURLcode res = CURLE_FAILED_INIT;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;
 
  upload_ctx.lines_read = 0;
 
  curl = curl_easy_init();
  if (curl) {
    /* From & To must include '<>' */
    payload_text[0] = strdup_printf ("To: <%s>\n", to);
    payload_text[1] = strdup_printf ("From: <%s> (%s)\n", smtp_username, fullname);
    payload_text[2] = strdup_printf ("Subject: %s\n", subject);
    /* empty line to divide headers from body, see RFC5322 */
    payload_text[3] = "\n";
    payload_text[4] = strdup_printf ("%s\n", body);
    payload_text[5] = NULL;
 
    curl_easy_setopt(curl, CURLOPT_URL, smtp_server);
 
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
 
    curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_password);
 
    /* value for envelope reverse-path */
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_username);
 
    recipients = curl_slist_append(recipients, to);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
 
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
 
    /* Since the traffic will be encrypted, it is very useful
    * to turn on debug information within libcurl to see what 
    * is happening during the transfer.
    */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
 
    res = curl_easy_perform(curl);
 
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
 
    free (payload_text[0]);
    free (payload_text[1]);
    free (payload_text[2]);
    free (payload_text[4]);
  }
 
  return res;
}

void mailStatus(char *status, const char *fmt, ...)
{
        char buff[XAP_DATA_LEN];
	va_list ap;

	va_start(ap, fmt);
	err(fmt, ap); // Log it

        int len = sprintf(buff, "xap-header\n"
			   "{\n"
			   "v=12\n"
			   "hop=1\n"
			   "uid=%s\n"
			   "class=email\n"
			   "source=%s\n"
			   "}\n"
			   "mail.%s\n"
			   "{\n"
                          "text=", xapGetUID(), xapGetSource(), status);
	len += vsnprintf(&buff[len], XAP_DATA_LEN-len, fmt, ap);
	va_end(ap);
	len += snprintf(&buff[len], XAP_DATA_LEN-len, "\n}\n");

	if(len < XAP_DATA_LEN) {
	  xapSend(buff);
	} else {
		err("Buffer overflow %d/%d", len, XAP_DATA_LEN);
	}
}


void emailService(void *userData)
{
  char *to = xapGetValue("message","to");
  char *text = xapGetValue("message","text");
  char *subject = xapGetValue("message","subject");
  
  if(to == NULL || text == NULL || subject == NULL) return;
  
  int result_code = sendmail(to, subject, text); 

  if (result_code == CURLE_OK) {
    mailStatus("ok","Send mail is complete");
  } else {
    // http://curl.haxx.se/libcurl/c/libcurl-errors.html
    mailStatus("error","Send mail failed with error code: %d", result_code);
  }
}


void setupXAPini()
{
	xapInitFromINI("mail","dbzoo","Mail","00E1",interfaceName, inifile);
        ini_gets("mail","server","",smtp_server,sizeof(smtp_server),inifile);
        ini_gets("mail","from","",smtp_username,sizeof(smtp_username),inifile);
        ini_gets("mail","fullname","",fullname,sizeof(fullname),inifile);
	smtp_password = getINIPassword("mail","password", (char *)inifile);

	int err = 0;
	if (strlen(smtp_server) == 0) {
	  crit("%s : [mail] server=  <- not defined", inifile);
	  err = 1;
	}
	if (strlen(smtp_username) == 0) {
	  crit("%s : [mail] from=  <- not defined", inifile);
	  err = 1;
	}
	if (strlen(fullname) == 0) {
	  crit("%s : [mail] fullname=  <- not defined", inifile);
	  err = 1;
	}
	if (smtp_password == NULL || strlen(smtp_password) == 0) {
	  crit("%s : [mail] password=  <- not defined", inifile);
	  err = 1;
	}
	if(err)
	  exit(1);
}


int main(int argc, char *argv[])
{
        printf("\nMAIL Connector for xAP v12\n");
        printf("Copyright (C) DBzoo 2014\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);	
        setupXAPini();

        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", "email");
        xapAddFilter(&f, "message", "to", XAP_FILTER_ANY);
        xapAddFilterAction(&emailService, f, NULL);

        xapProcess();
	exit(0);
}
