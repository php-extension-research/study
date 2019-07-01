<?php

function task($n, $a, $b)
{
	if ($n === 1) {
		echo "coroutine[1] create successfully" . PHP_EOL;
	}
	if ($n === 2) {
		echo "coroutine[2] create successfully" . PHP_EOL;
	}
	
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 1, 'a', 'b');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 2, 'c', 'd');
