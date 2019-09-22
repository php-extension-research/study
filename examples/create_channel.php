<?php

$chan = new Study\Coroutine\Channel();
var_dump($chan);

$chan = new Study\Coroutine\Channel(-1);
var_dump($chan);

$chan = new Study\Coroutine\Channel(2);
var_dump($chan);