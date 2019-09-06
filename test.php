<?php

while (true) {
    Sgo(function () {
        $cid = Sco::getCid();
        var_dump($cid);
        usleep(100);
        var_dump($cid);
    });
}