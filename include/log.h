#ifndef LOG_H_
#define LOG_H_

#include "study.h"

#define ST_OK 0
#define ST_ERR -1

#define ST_LOG_BUFFER_SIZE 1024
#define ST_LOG_DATE_STRLEN  64

#define ST_DEBUG_MSG_SIZE 512
#define ST_TRACE_MSG_SIZE 512
#define ST_WARN_MSG_SIZE 512
#define ST_ERROR_MSG_SIZE 512

extern char st_debug[ST_DEBUG_MSG_SIZE];
extern char st_trace[ST_TRACE_MSG_SIZE];
extern char st_warn[ST_WARN_MSG_SIZE];
extern char st_error[ST_ERROR_MSG_SIZE];

#define stDebug(str, ...)                                                         \
    snprintf(st_debug, ST_DEBUG_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
    stLog_put(ST_LOG_DEBUG, st_debug);

#define stTrace(str, ...)                                                         \
    snprintf(st_trace, ST_TRACE_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
    stLog_put(ST_LOG_TRACE, st_trace);

#define stWarn(str, ...)                                                         \
    snprintf(st_error, ST_ERROR_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
    stLog_put(ST_LOG_WARNING, st_error);

#define stError(str, ...)                                                         \
    snprintf(st_error, ST_ERROR_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
    stLog_put(ST_LOG_ERROR, st_error); \
    exit(-1);

enum stLog_level
{
    ST_LOG_DEBUG = 0,
    ST_LOG_TRACE,
    ST_LOG_INFO,
    ST_LOG_NOTICE,
    ST_LOG_WARNING,
    ST_LOG_ERROR,
};

void stLog_put(int level, char *cnt);

#endif /* LOG_H_ */