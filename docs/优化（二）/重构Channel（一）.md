# 重构Channel（一）

这篇文章，我们来重构一下我们扩展的`Study\Coroutine\Channel`类。为什么需要重构呢？我们来看一个测试脚本：

```php
<?php

$chan = new Study\Coroutine\Channel();

var_dump($chan);
```

执行脚本如下：

```shell
~/codeDir/cppCode/study # php test.php
object(Study\Coroutine\Channel)#1 (2) {
  ["zchan"]=>
  UNKNOWN:0
  ["capacity"]=>
  int(2)
}
~/codeDir/cppCode/study #
```

我们发现，我们的`zchan`这个属性被打印出来了。这个属性本来是保存了我们的一个`C++`的`study::coroutine::Channel`对象。`UNKNOWN`的`zval`是`PHP`层不可访问的类型。

那么，这里就会有一个问题，把一个`C++`的对象设计成`PHP`的`property`，我们可能会面临扩展底层属性被`PHP`脚本改写的尴尬情况。我们来测试一下：

```php
<?php

$chan = new Study\Coroutine\Channel();
$chan->zchan = null;
var_dump($chan);
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php
object(Study\Coroutine\Channel)#1 (2) {
  ["zchan"]=>
  NULL
  ["capacity"]=>
  int(1)
}
~/codeDir/cppCode/study #
```

我们看到，我们直接修改这个属性值为`null`了。我们再进一步测试：

```php
<?php

$chan = new Study\Coroutine\Channel();
$chan->zchan = null;

Sgo(function () use ($chan)
{
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php
string(10) "push start"
Segmentation fault
~/codeDir/cppCode/study #
```

我们发现，直接段错误了。而且，这种问题在我们的`Server`接口也是存在的，只要是涉及到了`PHP`使用一个`C++`对象都会出现这个问题。所以，这个问题我们必须解决。下一章节，我们将会开始讲解一个比较`Hack`的技巧，自定义`PHP`对象。

[下一篇：重构Channel（二）](./《PHP扩展开发》-协程-重构Channel（二）.md)
