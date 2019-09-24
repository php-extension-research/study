--TEST--
push and then pop
--SKIPIF--
<?php if (!extension_loaded("study")) print "skip"; ?>
--FILE--
<?php

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan) {
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan) {
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});
?>
--EXPECT--
string(10) "push start"
bool(true)
string(9) "pop start"
string(11) "hello world"