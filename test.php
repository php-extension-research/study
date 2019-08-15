<?php

$t1 = time();

$cid = Sgo(function () {
    echo "before sleep" . PHP_EOL;
    // SCo::yield();
    SCo::sleep(1);
    echo "after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;

while (true)
{
    $t2 = time();
    if ($t2 - $t1 >= 1)
    {
        SCo::resume($cid);
        break;
    }
}