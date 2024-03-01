#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "ansi.h"

#define DEBUG_ERR 1
#define DEBUG_LOG 2
#define DEBUG_INFO 3

#if DEBUG == DEBUG_ERR
    #define debug(level, ...) {\
        if (level == DEBUG_ERR) {\
            printf("%s",ANSI_COLOR_RED);\
            printf(__VA_ARGS__);\
            printf("%s", ANSI_COLOR_RESET);\
        }\
    }
#elif DEBUG == DEBUG_LOG
    #define debug(level,...) {\
        if (level == DEBUG_ERR) printf("%s",ANSI_COLOR_RED);\
        if (level == DEBUG_LOG) printf("%s",ANSI_COLOR_YELLOW);\
        if (level <= DEBUG) {\
            printf(__VA_ARGS__);\
            printf("%s", ANSI_COLOR_RESET);\
        }\
    }
#elif DEBUG >= DEBUG_INFO
    #define debug(level,...) {\
        if (level == DEBUG_ERR) printf("%s",ANSI_COLOR_RED);\
        if (level == DEBUG_LOG) printf("%s",ANSI_COLOR_YELLOW);\
        if (level == DEBUG_INFO) printf("%s",ANSI_COLOR_GREEN);\
        if (level <= DEBUG) {\
            printf(__VA_ARGS__);\
            printf("%s", ANSI_COLOR_RESET);\
        }\
    }
#else
    #define debug(level,...)
#endif

#endif //__DEBUG_H__