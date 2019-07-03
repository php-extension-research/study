# 修复一些bug（二）

我们发现，我们每创建一个新的协程，我们都会加入到`Study::Coroutine::coroutines`这个无序字典里面。但是，在协程完全执行完毕之后（指的是协程`死亡`之后），我们并没有把这个协程删除，这显然是不合理的对吧。所以我们需要针对这个情况做些处理。

我们知道，底层协程的入口函数是`Study::Context::context_func`，所以，底层协程完全执行完毕之后，必然是会返回到这个函数，又因为我们需要在其他类里面进行删除操作，所以我们就需要新增一个成员变量`end_`在`Study::Context`这个类里来标识这个协程的上下文是否已经结束了。

我们先在`Study::Context`类里面进行声明，很显然刚创建协程上下文的时候，`end_`是`false`：

```cpp
protected:
    bool end_ = false;
```

然后，我们修改一下我们的`Study::Context::context_func`这个函数：

```cpp
void Context::context_func(void *arg)
{
    Context *_this = (Context *) arg;
    _this->fn_(_this->private_data_);
    _this->end_ = true;
    _this->swap_out();
}
```

我们新加了一行：

```cpp
_this->end_ = true;
```

表示`PHP`协程入口函数执行完毕之后（同样的道理，`PHP`协程入口函数返回，代表`PHP`协程死亡了），设置`end_`为`true`。

然后，我们需要提供一个接口给外界访问`end_`，在`Study::Context`里面：

```cpp
inline bool is_end()
{
  	return end_;
}
```

然后，我们就需要在所有`swap_in`这个协程上下文的后面，去检查这个协程的上下文是否`end_`了。在`Study::Coroutine::run()`和`Study::Coroutine::resume`里面都调用了`ctx.swap_in`。

所以我们修改后的代码如下：

```cpp
long run()
{
    long cid = this->cid;
    origin = current;
    current = this;
    ctx.swap_in();
    if (ctx.is_end())
    {
        cid = current->get_cid();
        printf("in run method: co[%ld] end\n", cid);
        current = origin;
        coroutines.erase(cid);
        delete this;
    }
    return cid;
}
```

和

```cpp
void Coroutine::resume()
{
    origin = current;
    current = this;
    ctx.swap_in();
    if (ctx.is_end())
    {
        cid = current->get_cid();
        printf("in resume method: co[%ld] end\n", cid);
        current = origin;
        coroutines.erase(cid);
        delete this;
    }
}
```

OK，代码改造完毕。

我们进行编译、安装：

```shell
make clean && make && make install
```

测试脚本如下：

```php
<?php

function task($arg)
{
	$cid = Study\Coroutine::getCid();
	echo "coroutine [{$cid}] create" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
$cid1 = Study\Coroutine::create('task', 'a');
echo "main coroutine" . PHP_EOL;
$cid2 = Study\Coroutine::create('task', 'b');
echo "main coroutine" . PHP_EOL;
```

执行结果：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine [1] create
in run method: co[1] end
main coroutine
coroutine [2] create
in run method: co[2] end
main coroutine
~/codeDir/cppCode/study # 
```

因为这是不带有`yield`的，所以是协程new完，run后直接就返回了，因此成功检测到`end_`是`true`的地方是在`run`方法里面。

我们再来看看如何在`resume`中检测到协程`end_`了，脚本如下：

```php
<?php

function task($arg)
{
	$cid = Study\Coroutine::getCid();
	echo "coroutine [{$cid}] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [{$cid}] create" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
$cid1 = Study\Coroutine::create('task', 'a');
echo "main coroutine" . PHP_EOL;
$cid2 = Study\Coroutine::create('task', 'b');
echo "main coroutine" . PHP_EOL;

Study\Coroutine::resume($cid1);
echo "main coroutine" . PHP_EOL;
Study\Coroutine::resume($cid2);
echo "main coroutine" . PHP_EOL;
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] create
in resume method: co[1] end
main coroutine
coroutine [2] create
in resume method: co[2] end
main coroutine
~/codeDir/cppCode/study # 
```

OK，符合预期。







