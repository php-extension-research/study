#ifndef STUDY_COROUTINE_SOCKET_H
#define STUDY_COROUTINE_SOCKET_H

#include "php_study.h"
#include "socket.h"
#include "error.h"
#include "coroutine_socket.h"

void php_study_init_socket_object(zval *zsocket, study::coroutine::Socket *socket);

#endif	/* STUDY_COROUTINE_SOCKET_H */