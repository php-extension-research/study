# 压测server（二）

现在，我们来压测一下我们的服务器，我们编写测试脚本：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        $buf = $serv->recv($connfd);
        $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
        $serv->send($connfd, $responseStr);
        $serv->close($connfd);
    }
});

Sco::scheduler();
```

然后启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后使用`ab`进行压测：

```shell
~/codeDir/cppCode/fsw # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   0.625 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    15997.52 [#/sec] (mean)
Time per request:       6.251 [ms] (mean)
Time per request:       0.063 [ms] (mean, across all concurrent requests)
Transfer rate:          1499.77 [Kbytes/sec] received
```

`qps`达到了`15997.52`，我们还是比较满意的。
