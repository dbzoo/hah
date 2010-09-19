/* $Id$
   Copyright (c) Brett England, 2010
 
   Logging loosely based on code/ideas from kloned.
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifndef _LOG_H
#define _LOG_H

#include <errno.h>
#include <string.h>

#define LOG_EMERG           0
#define LOG_ALERT           1
#define LOG_CRIT            2
#define LOG_ERR             3
#define LOG_WARNING         4
#define LOG_NOTICE          5
#define LOG_INFO            6
#define LOG_DEBUG           7

#define log_write(lev, ...) log_write_ex(lev, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define die(...) \
do { \
log_write(LOG_CRIT, __VA_ARGS__); \
exit(1); \
} while(0)

#define die_if(expr,...) if(expr) die(__VA_ARGS__)
#define err_if(expr,...) if(expr) err(__VA_ARGS__)
#define warning_if(expr,...) if(expr) warning(__VA_ARGS__)
#define debug_if(expr,...) if(expr) debug(__VA_ARGS__)
#define notice_if(expr,...) if(expr) notice(__VA_ARGS__)

#define emerg(...) log_write(LOG_EMERG, __VA_ARGS__)
#define alert(...) log_write(LOG_ALERT, __VA_ARGS__)
#define crit(...) log_write(LOG_CRIT, __VA_ARGS__)
#define err(...) log_write(LOG_ERR, __VA_ARGS__)
#define warning(...) log_write(LOG_WARNING, __VA_ARGS__)
#define notice(...) log_write(LOG_NOTICE, __VA_ARGS__)
#define info(...) log_write(LOG_INFO, __VA_ARGS__)
#define debug(...) log_write(LOG_DEBUG, __VA_ARGS__)

#define msg_strerror(label,...) \
do {\
label("errno %d (%s)", errno, strerror(errno));\
label(__VA_ARGS__);\
} while(0)

#define die_strerror(...)\
do {\
crit("errno %d (%s)", errno, strerror(errno));\
die(__VA_ARGS__);\
} while(0)

#define debug_strerror(...) msg_strerror(debug, __VA_ARGS)
#define info_strerror(...) msg_strerror(info, __VA_ARGS)
#define notice_strerror(...) msg_strerror(notice, __VA_ARGS)
#define warning_strerror(...) msg_strerror(warning, __VA_ARGS)
#define err_strerror(...) msg_strerror(err,__VA_ARGS__)
#define crit_strerror(...) msg_strerror(crit, __VA_ARGS)
#define alert_strerror(...) msg_strerror(alert, __VA_ARGS)
#define emerg_strerror(...) msg_strerror(emerg, __VA_ARGS)

void log_write_ex(int level, const char *file, int line, const char *func, const char *fmt, ...);
void setLoglevel(int level);

#endif
