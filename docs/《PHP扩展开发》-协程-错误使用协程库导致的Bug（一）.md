# 错误使用协程库导致的Bug（一）

我们编写一个测试脚本：

```php
<?php

while (true) {
    Sgo(function () {
        $cid = Sco::getCid();
        var_dump($cid);
        Sco::sleep(0.1);
        var_dump($cid);
    });

    Sco::scheduler();
}
```

这个程序意图很明显，就是不断的创建协程，然后在协程里面`sleep 0.1`秒。但是，我们执行脚本会发现问题：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(1)

```

创建完协程`1`之后，程序就卡死了。原因很简单，当协程`1` `sleep`的时候，此时的控制权给了我们的调度器。于是乎，陷入了无限循环当中。这显然是和我们之前写`PHP`不一致的，例如如下的代码：

```php
<?php

while (true) {
    Sgo(function () {
        $cid = Sco::getCid();
        var_dump($cid);
        usleep(100);
        var_dump($cid);
    });
}
```

这段代码是可以正常的跑的，但是是同步阻塞的：

```shell
int(36145)
int(36145)
int(36146)
^C
~/codeDir/cppCode/study #
```

这个`Bug`我们后面会去解决。

[下一篇：重构协程调度器模块](./《PHP扩展开发》-协程-重构协程调度器模块.md)
