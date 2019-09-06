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
            if ($msg == false)
            {
                if ($serv->errCode == 1002)
                {
                    var_dump($serv->errMsg);
                    break;
                }
            }
            $serv->send($connfd, $msg);
        }
    }
});

Sco::scheduler();