<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        while (1)
        {
            $msg = $serv->recv($connfd);
            $serv->send($connfd, $msg);
        }
    }
});

Sco::scheduler();