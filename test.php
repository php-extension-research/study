<?php

$chan = new Study\Coroutine\Channel();
$chan->zchan = null;

Sgo(function () use ($chan)
{
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});
