<?php

study_event_init();

Sgo(function ()
{
    $server = new Study\Coroutine\Server("127.0.0.1", 80);
    $server->set_handler(function (Study\Coroutine\Socket $conn) use($server) {
        $data = $conn->recv();
        $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
        $conn->send($responseStr);
        $conn->close();
        // Sco::sleep(0.01);
    });
    $server->start();
});

study_event_wait();