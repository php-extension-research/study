<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

while (1)
{
    $connfd = $serv->accept();
    while (1)
    {
        $buf = $serv->recv($connfd);
        if ($buf == false)
        {
            break;
        }
        $serv->send($connfd, "hello");
    }
}
