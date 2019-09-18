<?php

study_event_init();

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});
study_event_wait();
