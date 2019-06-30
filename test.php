<?php

Study\Coroutine::create(function ($a, $b){
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}, 'a', 'b');