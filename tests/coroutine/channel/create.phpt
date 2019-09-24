--TEST--
create coroutine channnel
--SKIPIF--
<?php if (!extension_loaded("study")) print "skip"; ?>
--FILE--
<?php 

$chan = new Study\Coroutine\Channel();
var_dump($chan->capacity);

$chan = new Study\Coroutine\Channel(-1);
var_dump($chan->capacity);

$chan = new Study\Coroutine\Channel(2);
var_dump($chan->capacity);
?>
--EXPECT--
int(1)
int(1)
int(2)