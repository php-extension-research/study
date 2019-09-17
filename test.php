<?php

study_event_init();

Sgo(function () {
    var_dump(1);
    Sco::sleep(1);
    var_dump(2);
});

Sgo(function () {
    var_dump(3);
    Sco::sleep(1);
    var_dump(4);
});

study_event_wait();