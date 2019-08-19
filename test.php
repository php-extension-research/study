<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

$sock = $serv->accept();
$buf = $serv->recv($sock);
$serv->send($sock, $buf);
