#include "socket.h"

int stSocket_create(int type)
{
    int _domain;
    int _type;

    if (type == ST_SOCK_TCP)
    {
        _domain = AF_INET;
        _type = SOCK_STREAM;
    }
    else if (type == ST_SOCK_UDP)
    {
        _domain = AF_INET;
        _type = SOCK_DGRAM;
    }
    else
    {
        return -1;
    }

    return socket(_domain, _type, 0);
}

int stSocket_bind(int sock, int type, char *host, int port)
{
    int ret;
    struct sockaddr_in servaddr;

    if (type == ST_SOCK_TCP)
    {
        bzero(&servaddr, sizeof(servaddr));
        inet_aton(host, &(servaddr.sin_addr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        ret = bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (ret < 0)
        {
            return -1; 
        }
    }
    else
    {
        return -1;
    }

    return ret;
}