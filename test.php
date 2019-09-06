<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        var_dump($connfd);
        $serv->close($connfd);
    }
});

Sco::scheduler();