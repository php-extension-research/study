<?php

$cid = Study\Coroutine::getCid();
echo "coroutine [{$cid}] create" . PHP_EOL;

// function task($arg)
// {
// 	$cid = Study\Coroutine::getCid();
// 	echo "coroutine [{$cid}] create" . PHP_EOL;
// 	Study\Coroutine::yield();
// 	echo "coroutine [{$cid}] is resumed" . PHP_EOL;
// }

// echo "main coroutine" . PHP_EOL;
// $cid1 = Study\Coroutine::create('task', 'a');
// echo "main coroutine" . PHP_EOL;
// $cid2 = Study\Coroutine::create('task', 'b');
// echo "main coroutine" . PHP_EOL;
// Study\Coroutine::resume($cid1);
// echo "main coroutine" . PHP_EOL;
// Study\Coroutine::resume($cid2);
// echo "main coroutine" . PHP_EOL;
