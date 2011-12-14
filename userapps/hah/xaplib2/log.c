/* $Id$
   Copyright (c) Brett England, 2010
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static int loglevel = LOG_ERR;

void setLoglevel(int level)
{
        loglevel = level;
}

int getLoglevel() {
	return loglevel;
}

static inline const char* log_label(int lev)
{
        switch(lev) {
        case LOG_DEBUG:
                return "dbg";
        case LOG_INFO:
                return "inf";
        case LOG_NOTICE:
                return "ntc";
        case LOG_WARNING:
                return "wrn";
        case LOG_ERR:
                return "err";
        case LOG_CRIT:
                return "crt";
        case LOG_ALERT:
                return "alr";
        case LOG_EMERG:
                return "emg";
        default:
                warning("unknown log level: %d", lev);
                return "unk";
        }
}

void log_write_ex(int level, const char *file, int line, const char *func, const char *fmt, ...)
{
	if(level > loglevel) return;

        va_list ap;
        char buf[2048];

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        printf("[%s][%s:%d:%s] %s\n", log_label(level), file, line, func, buf);
}
