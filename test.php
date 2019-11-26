<?php

study_event_init();

Study\Runtime::enableCoroutine();

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
    sleep(1);
    var_dump(Study\Coroutine::getCid());
});

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
});

study_event_wait();