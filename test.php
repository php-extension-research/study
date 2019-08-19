<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
$sock = $serv->accept();
var_dump($sock);
