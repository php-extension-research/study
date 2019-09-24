--TEST--
sleep 2
--SKIPIF--
<?php if (!extension_loaded("study")) print "skip"; ?>
--FILE--
<?php

study_event_init();

Sgo(function ()
{
    var_dump(Sco::getCid());
    Sco::sleep(0.02);
    var_dump(Sco::getCid());
});

Sgo(function ()
{
    var_dump(Sco::getCid());
    Sco::sleep(0.01);
    var_dump(Sco::getCid());
});

study_event_wait();
?>
--EXPECT--
int(1)
int(2)
int(2)
int(1)