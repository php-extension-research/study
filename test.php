<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        Sgo(function () use ($serv, $connfd)
        {
            while (1)
            {
                $msg = $serv->recv($connfd);
                var_dump($msg);
            }
        });
    }
});

Sco::scheduler();
