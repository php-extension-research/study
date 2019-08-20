<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

$sock = $serv->accept();

while (1)
{
    $buf = $serv->recv($sock);
    if ($buf == false)
    {
        var_dump($serv->errCode);
        var_dump($serv->errMsg);
        break;
    }
    $serv->send($sock, $buf);
}
