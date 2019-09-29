# 重构Channel（二）

在上一篇文章，我们解释了为什么把一个`C++`对象直接存成一个`PHP`对象的属性容易造成`bug`。所以，我们就不可以把一个`C++`对象存成一个`PHP`对象的属性，我们应该只把那些基本的属性、和`PHP`对象强相关的东西存成`PHP`对象的属性。

那么，为什么我们当初要把一个`C++`的对象存储在`PHP`对象的一个属性里面呢？因为，我们扩展的每一个`PHP`对象，底层其实都是对应一个`C++`的对象。而我们的扩展是不能直接把一个`C++`对象`RETUEN`给`PHP`脚本使用，我们看到了，那个属性是一个`UNKNOWN`类型的。所以，为了让我们的`PHP`脚本在调用`PHP`对象的方法的时候可以找到和这个`PHP`对象对应的那个`C++`对象，我们把`C++`对象存储为了`PHP`对象的一个属性，以便我们在扩展层通过读`PHP`属性得到对应的`C++`对象，然后实现后续的功能。

所以，我们现在需要解决的问题就是如何通过一个`PHP`对象找到一个`C++`对象？这篇文章，我们来实现自定义的`Channel`对象。通过自定义`PHP`对象，可以轻松的实现这个功能。

在文件`study_coroutine_channel.cc`里面，我们定义一个`PHP`对象的`handlers`：

```cpp
static zend_object_handlers study_coro_channel_handlers;
```

这个`zend_object_handlers`实际上就是我们在`PHP`脚本上面操作一个`PHP`对象的时候，底层会去调用的函数。

所以，我们这里要实现一个自定义的`PHP`对象，是需要去实现这些方法的。所幸的是，`PHP`底层已经为我们实现了默认的方法，我们只需要修改我们需要的那些`handler`即可，其他的保持默认的进行了。

然后，我们定义我们自定义的`PHP`对象：

```cpp
typedef struct
{
    Channel *chan;
    zend_object std;
} coro_chan;
```

其中，`Channel *chan;`就是对应我们的一个`C++`对象，而`zend_object std;`就是对应我们的一个`PHP`对象。等下，我们将会讲解如何通过`std`这个`PHP`对象找到它上面的`chan`对象。（理解这个需要理解`C++`中结构体的内存布局）

接下来，我们就去实现通过这个`PHP`对象`std`找到我们的`C++`对象的代码：

```cpp
static coro_chan* study_coro_channel_fetch_object(zend_object *obj)
{
    return (coro_chan *)((char *)obj - study_coro_channel_handlers.offset);
}
```

其中，这里传入的参数`obj`实际上就对应我们的`std`。我们必须明白，在一个结构体里面，属性与属性之间内存是连续的（简单分析，不考虑内存对齐），所以，我们可以直接通过地址的加法和减法计算出同一个结构体里面其他属性的地址。而`study_coro_channel_handlers.offset`则是`chan`与`std`两个属性的地址偏移量，这个偏移量我们在其他地方会去计算出来，后面会去实现。所以，这里我们就通过了指针计算的方法得到了`chan`的地址。

接着，我们去实现创建自定义`PHP`对象的函数：

```cpp
static zend_object* study_coro_channel_create_object(zend_class_entry *ce)
{
    coro_chan *chan_t = (coro_chan *)ecalloc(1, sizeof(coro_chan) + zend_object_properties_size(ce));
    zend_object_std_init(&chan_t->std, ce);
    object_properties_init(&chan_t->std, ce);
    chan_t->std.handlers = &study_coro_channel_handlers;
    return &chan_t->std;
}
```

如果读过`PHP`内核代码的话，这段代码其实和`PHP`对象的默认创建方法类似，对应如下函数：

```c
ZEND_API zend_object *zend_objects_new(zend_class_entry *ce)
{
    zend_object *object = emalloc(sizeof(zend_object) + zend_object_properties_size(ce));
    zend_object_std_init(object, ce);
    object->handlers = &std_object_handlers;
    return object;
}
```

其中，

我们得理解`zend_objects_new`这个函数的作用是什么？实际上，之前我们写过一个和这个函数很容易混淆的一个函数`__construct`。没错，就是一个类的构造函数。我们一定要明确一点，这个构造函数不是去创建一个`PHP`对象的，而是去初始化一个`PHP`对象。例如，初始化`PHP`对象的属性等等。在调用构造函数之前，实际上这个`PHP`对象就已经被创建好了，只不过这个`PHP`对象可能是一个只有默认值的属性的对象。

而在构造函数之前被调用的，就是这个`zend_objects_new`。而`zend_objects_new`是一个`PHP`默认的创建对象的`handler`，我们不可能直接去修改这个默认的`handler`的源码。所幸的是，我们可以替换掉这个`handler`，从而实现我们的自定义对象，只要这个`handler`的参数以及返回值符合接口规范即可。我们可以看到，`zend_objects_new`的参数是一个`zend_class_entry`，也就是一个`PHP`的类，然后返回值是一个`zend_object`，也就是一个`PHP`对象。

现在，我们可以解释我们自己的`study_coro_channel_create_object`函数了。

其中，

```cpp
zend_object *object = emalloc(sizeof(zend_object) + zend_object_properties_size(ce));
```

创建了我们的自定义对象`coro_chan`。有因为`coro_chan`里面的`std`不是指针，所以，我们在分配`coro_chan`对象的内存的时候，也把`PHP`对象的内存分配了。所以，我们在最后一行代码，可以直接返回这个`PHP`对象：

```cpp
return &chan_t->std;
```

我们来解释一下分配的内存大小为什么不是`zend_object`而是`zend_object + zend_object_properties_size`。这个就需要去看`PHP`中`zend_object`的定义了：

```c
struct _zend_object {
    zend_refcounted_h gc;
    uint32_t          handle; // TODO: may be removed ???
    zend_class_entry *ce;
    const zend_object_handlers *handlers;
    HashTable        *properties;

    /**
     * 成员属性数组，
     * 因为这个数组是一个柔性数组，所以zend_object是个变长结构体，分配时会根据非静态属性的数量确定zend_object的大小。
     */
    zval              properties_table[1];
};
```

我们发现，`_zend_object`这个结构体最后一个属性就是用来存放`PHP`对象属性的，但是因为这个数组是一个柔性数组，所以zend_object是个变长结构体，分配时会根据非静态属性的数量确定zend_object的大小。如果我们直接分配`zend_object`的大小，就会把`PHP`对象的属性给漏掉。这是实现自定义对象需要特别关注的问题。如果还是不理解，小伙伴们可以先去学习`C`语言的柔性数组。

接下来：

```cpp
zend_object_std_init(&chan_t->std, ce);
object_properties_init(&chan_t->std, ce);
chan_t->std.handlers = &study_coro_channel_handlers;
```

这就是标准的操作了，给创建出来的`PHP`对象本身进行初始化，以及属性的初始化，以及`handler`的初始化。

接下来，我们去实现一下释放`PHP`对象的方法：

```cpp
static void study_coro_channel_free_object(zend_object *object)
{
    coro_chan *chan_t = (coro_chan *)study_coro_channel_fetch_object(object);
    Channel *chan = chan_t->chan;
    if (chan)
    {
        while (!chan->empty())
        {
            zval *data;
            data = (zval *)chan->pop_data();
            zval_ptr_dtor(data);
            efree(data);
        }

        delete chan;
    }
    zend_object_std_dtor(&chan_t->std);
}
```

这里需要注意的一个问题就是不要忘记释放`Channel`里面的`data`。因为如果一个`Channel`用不到了，里面的数据自然也是用不到了。这里，我们需要实现`chan->empty`以及`chan->pop_data()`方法。在文件`include/coroutine_channel.h`里面进行声明：

```cpp
public:
    bool empty();
    void* pop_data();
```

然后在文件`src/coroutine/channel.cc`里面去实现：

```cpp
bool Channel::empty()
{
    return data_queue.empty();
}

void* Channel::pop_data()
{
    void *data;

    if (data_queue.size() == 0)
    {
        return nullptr;
    }
    data = data_queue.front();
    data_queue.pop();
    return data;
}
```

代码很简单，不多啰嗦。

最后，我们只需要修改我们的接口即可，首先是构造函数：

```cpp
static PHP_METHOD(study_coro_channel, __construct)
{
    coro_chan *chan_t;
    zend_long capacity = 1;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(capacity)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (capacity <= 0)
    {
        capacity = 1;
    }

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan_t->chan = new Channel(capacity);

    zend_update_property_long(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("capacity"), capacity);
}
```

然后，修改`push`接口：

```cpp
static PHP_METHOD(study_coro_channel, push)
{
    coro_chan *chan_t;
    Channel *chan;
    zval *zdata;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(zdata)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan = chan_t->chan;

    Z_TRY_ADDREF_P(zdata);
    zdata = st_zval_dup(zdata);

    if (!chan->push(zdata, timeout))
    {
        Z_TRY_DELREF_P(zdata);
        efree(zdata);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
```

然后修改`pop`接口：

```cpp
static PHP_METHOD(study_coro_channel, pop)
{
    coro_chan *chan_t;
    Channel *chan;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan = chan_t->chan;
    zval *zdata = (zval *)chan->pop(timeout);
    if (!zdata)
    {
        RETURN_FALSE;
    }
    RETVAL_ZVAL(zdata, 0, 0);
    efree(zdata);
}
```

然后，在模块初始化的地方，我们需要去替换掉`PHP`默认的`handler`：

```cpp
void study_coro_channel_init()
{
    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine
    memcpy(&study_coro_channel_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_channel, study_coro_channel_create_object, study_coro_channel_free_object, coro_chan, std);

    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}
```

其中，`ST_SET_CLASS_CUSTOM_OBJECT`用来替换默认的创建`PHP`对象以及释放`PHP`对象的`handler`。并且，`PHP`对象与`C++`对象的地址偏移量也是通过这个宏计算得到的。我们在文件`php_study.h`定义这个宏：

```cpp
#define ST_SET_CLASS_CREATE(module, _create_object) \
    module##_ce_ptr->create_object = _create_object

#define ST_SET_CLASS_FREE(module, _free_obj) \
    module##_handlers.free_obj = _free_obj

#define ST_SET_CLASS_CREATE_AND_FREE(module, _create_object, _free_obj) \
    ST_SET_CLASS_CREATE(module, _create_object); \
    ST_SET_CLASS_FREE(module, _free_obj)

/**
 * module##_handlers.offset 保存PHP对象在自定义对象中的偏移量
 */
#define ST_SET_CLASS_CUSTOM_OBJECT(module, _create_object, _free_obj, _struct, _std) \
    ST_SET_CLASS_CREATE_AND_FREE(module, _create_object, _free_obj); \
    module##_handlers.offset = XtOffsetOf(_struct, _std)
```

这里补充一点，因为`chan`这个`C++`对象是自定义对象的第一个参数，所以，我们可以说`std`与`chan`的偏移量实际上就是`std`在结构体中的偏移量。所以，准确来说，`handlers.offset`里面保存的是`std`在结构体中的偏移量，然后我们后面通过结构体的地址，找到`chan`这个`C++`对象的地址。

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后编写测试脚本：

```shell
<?php

$chan = new Study\Coroutine\Channel();

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
bool(true)
string(9) "pop start"
string(11) "hello world"
~/codeDir/cppCode/study #
```

我们发现，程序可以跑通。接着，我们来看看那个属性问题是否得到解决。测试脚本如下：

```php
<?php

$chan = new Study\Coroutine\Channel();

var_dump($chan);
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php
object(Study\Coroutine\Channel)#1 (1) {
  ["capacity"]=>
  int(1)
}
~/codeDir/cppCode/study #
```

符合预期。

[下一篇：自定义Socket对象](./《PHP扩展开发》-协程-自定义Socket对象.md)
