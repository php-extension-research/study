<?php

study_event_init();

while (1) {
    Sgo(function () {
        var_dump(Sco::getCid());
        Sco::sleep(1);
        var_dump(Sco::getCid());
    });
    study_event_wait();
}
