# 教程

## 开发环境

PHP版本：7.3.5

操作系统：我希望你是`alpine`

你也可以直接使用我为你准备的`Dockerfile`，以保证和我的开发环境一致，避免不必要的麻烦：

```shell
docker build -t study -f docker/Dockerfile .
```

## 《PHP扩展开发》--协程

### 第一阶段 协程基础模块

[1、编写config.m4文件](./docs/《PHP扩展开发》-协程-编写config-m4文件.md)

[2、开发规范](./docs/《PHP扩展开发》-协程-开发规范.md)

[3、整理文件](./docs/《PHP扩展开发》-协程-整理文件.md)

[4、理解PHP生命周期的过程](./docs/《PHP扩展开发》-协程-理解PHP生命周期的过程.md)

[5、梳理一下架构](./docs/《PHP扩展开发》-协程-梳理一下架构.md)

[6、协程创建（一）](./docs/《PHP扩展开发》-协程-协程创建（一）.md)

[7、协程创建（二）](./docs/《PHP扩展开发》-协程-协程创建（二）.md)

[8、协程创建（三）](./docs/《PHP扩展开发》-协程-协程创建（三）.md)

[9、协程创建（四）](./docs/《PHP扩展开发》-协程-协程创建（四）.md)

[10、协程创建（五）](./docs/《PHP扩展开发》-协程-协程创建（五）.md)

[11、协程创建（六）](./docs/《PHP扩展开发》-协程-协程创建（六）.md)

[12、协程创建（七）](./docs/《PHP扩展开发》-协程-协程创建（七）.md)

[13、协程创建（八）](./docs/《PHP扩展开发》-协程-协程创建（八）.md)

[14、协程yield](./docs/《PHP扩展开发》-协程-协程yield.md)

[15、协程resume](./docs/《PHP扩展开发》-协程-协程resume.md)

[16、协程getCid](./docs/《PHP扩展开发》-协程-协程getCid.md)

[17、修复一些bug（一）](./docs/《PHP扩展开发》-协程-修复一些bug（一）.md)

[18、修复一些bug（二）](./docs/《PHP扩展开发》-协程-修复一些bug（二）.md)

[19、协程isExist](./docs/《PHP扩展开发》-协程-协程isExist.md)

[20、修复一些bug（三）](./docs/《PHP扩展开发》-协程-修复一些bug（三）.md)

[21、协程defer](./docs/《PHP扩展开发》-协程-协程defer.md)

[22、协程短名（一）](./docs/《PHP扩展开发》-协程-协程短名（一）.md)

[23、协程短名（二）](./docs/《PHP扩展开发》-协程-协程短名（二）.md)

到目前为止，我们已经实现了协程常用的关键接口，可以算是本书的第一版吧，接下来，我将会带领大家去实现网络的部分。我们的最终目标是去实现一个高性能的协程化的服务器。因为这其中涉及到了比较多的数据结构，所以需要些时间来构思文章内容。

大家可以提前去学习下以下知识点：IO多路复用（重点学习`epoll`）、数据结构中的堆、定时器。

敬请期待。

### 第二阶段 网络模块

[24、引入libuv](./docs/《PHP扩展开发》-协程-引入libuv.md)

[25、sleep（一）](./docs/《PHP扩展开发》-协程-sleep（一）.md)

[26、sleep（二）](./docs/《PHP扩展开发》-协程-sleep（二）.md)

[27、sleep（三）](./docs/《PHP扩展开发》-协程-sleep（三）.md)

[28、sleep（四）](./docs/《PHP扩展开发》-协程-sleep（四）.md)

[29、sleep（五）](./docs/《PHP扩展开发》-协程-sleep（五）.md)

[30、server创建（一）](./docs/《PHP扩展开发》-协程-server创建（一）.md)

[31、server创建（二）](./docs/《PHP扩展开发》-协程-server创建（二）.md)

[32、server接收请求](./docs/《PHP扩展开发》-协程-server接收请求.md)

[33、server监听的封装](./docs/《PHP扩展开发》-协程-server监听的封装.md)

[34、server接收数据](./docs/《PHP扩展开发》-协程-server接收数据.md)

[35、server发送数据](./docs/《PHP扩展开发》-协程-server发送数据.md)

[36、server错误码](./docs/《PHP扩展开发》-协程-server错误码.md)

[37、压测server（一）](./docs/《PHP扩展开发》-协程-压测server（一）.md)

[38、socket可读写时候调度协程的思路](./docs/《PHP扩展开发》-协程-socket可读写时候调度协程的思路.md)

[39、全局变量STUDYG](./docs/《PHP扩展开发》-协程-全局变量STUDYG.md)

[40、定义协程化的Socket类](./docs/《PHP扩展开发》-协程-定义协程化的Socket类.md)

[41、协程化Socket::Socket](./docs/《PHP扩展开发》-协程-协程化Socket::Socket.md)

[42、实现coroutine::Socket::bind和listen](./docs/《PHP扩展开发》-协程-实现coroutine::Socket::bind和listen.md)

[43、协程化Socket::accept](./docs/《PHP扩展开发》-协程-协程化Socket::accept.md)

[44、协程化Socket::wait_event](./docs/《PHP扩展开发》-协程-协程化Socket::wait_event.md)

[45、在事件到来时resume对应的协程](./docs/《PHP扩展开发》-协程-在事件到来时resume对应的协程.md)