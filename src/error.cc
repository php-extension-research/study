#include "error.h"
#include "log.h"

const char* st_strerror(int code)
{
    switch (code)
    {
    case ST_ERROR_SESSION_CLOSED_BY_SERVER:
        return "Session closed by server";
        break;
    case ST_ERROR_SESSION_CLOSED_BY_CLIENT:
        return "Session closed by client";
        break;
    default:
        snprintf(st_error, sizeof(st_error), "Unknown error: %d", code);
        return st_error;
        break;
    }
}