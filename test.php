<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
        }, $serv, $connfd);
    }
});

study_event_wait();
