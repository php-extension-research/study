#ifndef STUDY_COROUTINE_SERVER_H
#define STUDY_COROUTINE_SERVER_H

#include "php_study.h"
#include "study_coroutine.h"
#include "socket.h"
#include "coroutine_socket.h"
#include "error.h"

namespace study { namespace phpcoroutine {
class Server
{
private:
    study::coroutine::Socket *socket = nullptr;
    php_study_fci_fcc *handler = nullptr;
    bool running = false;

public:
    Server(char *host, int port);
    ~Server();
    bool start();
    bool shutdown();
    void set_handler(php_study_fci_fcc *_handler);
    php_study_fci_fcc* get_handler();
};
}
}

#endif /* STUDY_COROUTINE_SERVER_H */