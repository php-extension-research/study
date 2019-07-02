<?php

function task($n, $arg)
{
	echo "coroutine [$n] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [$n] be resumed" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 1, 'a');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 2, 'b');
echo "main coroutine" . PHP_EOL;

Study\Coroutine::resume(1);
echo "main coroutine" . PHP_EOL;
Study\Coroutine::resume(2);
echo "main coroutine" . PHP_EOL;