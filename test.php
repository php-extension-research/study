<?php

function deferFunc1()
{
    echo "in defer deferFunc1" . PHP_EOL;
}

function deferFunc2()
{
    echo "in defer deferFunc2" . PHP_EOL;
}

function deferFunc3()
{
    echo "in defer deferFunc3" . PHP_EOL;
}

function task()
{
    echo "task coroutine start" . PHP_EOL;
    Co::defer('deferFunc1');
    Co::defer('deferFunc2');
    Co::defer('deferFunc3');
    echo "task coroutine end" . PHP_EOL;
}

$cid1 = Co::create('task');