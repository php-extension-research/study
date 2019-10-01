<?php

study_event_init();

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan) {
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});

Sgo(function () use ($chan) {
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

study_event_wait();
