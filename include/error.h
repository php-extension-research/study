#ifndef ERROR_H
#define ERROR_H

#include "study.h"

enum stErrorCode
{
    /**
     * connection error
     */
    ST_ERROR_SESSION_CLOSED_BY_SERVER = 1001,
    ST_ERROR_SESSION_CLOSED_BY_CLIENT,
};

const char* st_strerror(int code);

#endif	/* ERROR_H */
