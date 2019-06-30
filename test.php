<?php

function task($a, $b)
{
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}

Study\Coroutine::create('task', 'a', 'b');
Study\Coroutine::create(function ($a, $b) {
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}, 'a', 'b');
