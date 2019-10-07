--TEST--
create coroutine socket
--SKIPIF--
<?php if (!extension_loaded("study")) print "skip"; ?>
--FILE--
<?php 

$socket = new Study\Coroutine\Socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
var_dump($socket);
?>
--EXPECT--
object(Study\Coroutine\Socket)#1 (3) {
  ["fd"]=>
  int(3)
  ["errCode"]=>
  int(0)
  ["errMsg"]=>
  string(0) ""
}