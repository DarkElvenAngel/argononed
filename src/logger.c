/*
MIT License

Copyright (c) 2022 DarkElvenAngel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "argononed.common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * \brief Log level to string
 * 
 */
const char* LOG_LEVEL_STR[7] = {"NONE", "FATAL", "CRITICAL", "ERROR", "WARNING", "INFO",  "DEBUG"};

/**
 * \brief Convert log level to colour
 * 
 */
const uint8_t LOG_COLOUR[6] = { 91, 95, 31, 93, 96, 92 };
extern struct SHM_Data* ptr;
extern struct DTBO_Data Configuration;

/**
 * \brief Write formatted output to Log file.
 * 
 * \param level Message's Log level
 * \param message formatted text to output
 * \return none
 */
void log_message(Log_Level l, const char *message, ...)
{
   /*  printf("Loglevel set to %s level %d %s FORGROUND MODE = %d\n", 
        LOG_LEVEL_STR[Configuration.Log_Level],
        level,
        LOG_LEVEL_STR[level],
        Configuration.extra.flags.FOREGROUND_MODE
    ); */
    uint8_t level = l & 0x7;
    if (!Configuration.colour) l &= 0x7;
    FILE *logfile;
    const uint8_t Foreground_Mode = Configuration.extra.flags.FOREGROUND_MODE;
    va_list args;
    if (ptr && (level <= LOG_WARN))
    {
        if (level == LOG_WARN) ptr->stat.EF_Warning++;
        if (level == LOG_ERROR) ptr->stat.EF_Error++;
        if (level == LOG_CRITICAL) ptr->stat.EF_Critical ++;
    }
#ifndef RUN_IN_FOREGROUND
    if (!Foreground_Mode)
    {
        logfile = fopen(LOG_FILE,"a");
    } else {
        logfile = stdout;
    }
#else
        logfile = stdout;
#endif
    if(!logfile) return;
    if (level <= (Configuration.Log_Level) )
    {
        time_t now;
        time(&now);
        char * date = ctime(&now);
        date[strlen(date) - 1] = '\0';
        if (Configuration.colour)
        {
            fprintf(logfile,"%s [\e[%dm %s \e[0m] ", date, LOG_COLOUR[level - 1], LOG_LEVEL_STR[level]);
        } else {
            fprintf(logfile,"%s [ %s ] ", date, LOG_LEVEL_STR[level]);
        }
        if (l & LOG_BOLD) fprintf(logfile,"\e[1m");
        if (l & LOG_INVERT) fprintf(logfile,"\e[7m");
        va_start(args, message);
        vfprintf(logfile, message, args);
        va_end(args);
        if (l & 0x18) fprintf(logfile,"\e[0m");
        fprintf(logfile,"\n");
    }
#ifndef RUN_IN_FOREGROUND
    if (!Foreground_Mode) fclose(logfile);
#endif
}