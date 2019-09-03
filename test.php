<?php

Sgo(function ()
{
    var_dump("here1");
    Sco::sleep(0.5);
    var_dump("here2");
    Sco::sleep(1);
    var_dump("here3");
    Sco::sleep(1.5);
    var_dump("here4");
    Sco::sleep(2);
    var_dump("here5");
});

Sco::scheduler();
