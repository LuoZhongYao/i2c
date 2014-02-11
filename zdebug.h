#ifndef __ZDEBUG_H__
#define __ZDEBUG_H__

#include    <stdio.h>

#define DEBUG
#define TREM_COLOR

#ifdef  TREM_COLOR
#define WARNING_COLOR   "\e[31m"
#define MESSAGE_COLOR   "\e[32m"
#define DEBUG_COLOR     "\e[34m"
#define DEFAULT_COLOR   "\e[0m"
#define MAKE_COLOR(color,message)   color message DEFAULT_COLOR
#else
#define WARNING_COLOR
#define MESSAGE_COLOR
#define MAKE_COLOR(color,message)   message
#endif

#ifdef  DEBUG
#define dprint(fmt,...) fprintf(stdout,MAKE_COLOR(DEBUG_COLOR,"[   I2C] : %4d ") fmt MAKE_COLOR(DEBUG_COLOR," *%s*\n"),__LINE__,##__VA_ARGS__,__func__)
#define eprint(fmt,...) fprintf(stderr,MAKE_COLOR(DEBUG_COLOR,"[   I2C] : %4d ") fmt MAKE_COLOR(DEBUG_COLOR," *%s*\n"),__LINE__,##__VA_ARGS__,__func__)
#else
#define dprint(fmt,...)
#define eprint(fmt,...)
#endif
#define message(fmt,...)    dprint(MAKE_COLOR(MESSAGE_COLOR,"*M* ")fmt,##__VA_ARGS__)
#define warning(fmt,...)    eprint(MAKE_COLOR(WARNING_COLOR,"*W* ")fmt,##__VA_ARGS__)

#endif
