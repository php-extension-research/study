<?php

$c = 'c';
$d = 'd';

Study\Coroutine::create(function ($a, $b) use ($c, $d) {
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
	echo $c . PHP_EOL;
	echo $d . PHP_EOL;
}, 'a', 'b');