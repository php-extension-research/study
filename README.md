# 教程

## 感谢

感谢`Swoole`这个项目，没有这个项目，就没有这个教程。

## 特别注意

请不要安装`xdebug`扩展。

## 本教程适合人群

懂**基本的**数据结构、网络编程、汇编语言、C语言，能读懂代码即可，并且希望成为一个优秀的`PHP`工程师的人。

## 讨论

可以加`QQ`群进行讨论：

```shell
942858122
```

如果对教程有什么疑问，欢迎在`issue`提出（建议先提`issue`，然后再在群里问），我们一起努力打造出一个高质量的、系统化的`PHP`扩展开发教程。

## 开发环境

PHP版本：7.3.5

操作系统：我希望你是`alpine`

你也可以直接使用我为你准备的`Dockerfile`，以保证和我的开发环境一致，避免不必要的麻烦：

```shell
docker build -t study -f docker/Dockerfile .
```

## 目录

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

[46、协程化Socket::recv和send](./docs/《PHP扩展开发》-协程-协程化Socket::recv和send.md)

[47、实现coroutine::Socket::close](./docs/《PHP扩展开发》-协程-实现coroutine::Socket::close.md)

[48、协程化服务器（一）](./docs/《PHP扩展开发》-协程-协程化服务器（一）.md)

[49、协程化服务器（二）](./docs/《PHP扩展开发》-协程-协程化服务器（二）.md)

[50、协程化服务器（三）](./docs/《PHP扩展开发》-协程-协程化服务器（三）.md)

[51、协程化服务器（四）](./docs/《PHP扩展开发》-协程-协程化服务器（四）.md)

### 第三阶段 优化（一）

[52、修复一些bug（四）](./docs/《PHP扩展开发》-协程-修复一些bug（四）.md)

[53、修复一些bug（五）](./docs/《PHP扩展开发》-协程-修复一些bug（五）.md)

[54、修复一些bug（六）](./docs/《PHP扩展开发》-协程-修复一些bug（六）.md)

[55、server关闭连接](./docs/《PHP扩展开发》-协程-server关闭连接.md)

[56、压测server（二）](./docs/《PHP扩展开发》-协程-压测server（二）.md)

[57、修复一些bug（七）](./docs/《PHP扩展开发》-协程-修复一些bug（七）.md)

[58、错误使用协程库导致的Bug（一）](./docs/《PHP扩展开发》-协程-错误使用协程库导致的Bug（一）.md)

[59、重构协程调度器模块](./docs/《PHP扩展开发》-协程-重构协程调度器模块.md)

[60、修复一些bug（八）](./docs/《PHP扩展开发》-协程-修复一些bug（八）.md)

[61、修复一些bug（九）](./docs/《PHP扩展开发》-协程-修复一些bug（九）.md)

[62、重构定时器（一）](./docs/《PHP扩展开发》-协程-重构定时器（一）.md)

[62、重构定时器（二）](./docs/《PHP扩展开发》-协程-重构定时器（二）.md)

[63、保存PHP栈](./docs/《PHP扩展开发》-协程-保存PHP栈.md)

### 第四阶段 CSP并发模型

[64、Channel实现原理](./docs/《PHP扩展开发》-协程-Channel实现原理.md)

[65、Channel创建](./docs/《PHP扩展开发》-协程-Channel创建.md)

[66、实现Channel基础类](./docs/《PHP扩展开发》-协程-实现Channel基础类.md)

[67、Channel的push和pop](./docs/《PHP扩展开发》-协程-Channel的push和pop.md)

### 第四阶段 优化（二）

[68、修复一些bug（十）](./docs/《PHP扩展开发》-协程-修复一些bug（十）.md)

[69、重构Channel（一）](./docs/《PHP扩展开发》-协程-重构Channel（一）.md)

[70、重构Channel（二）](./docs/《PHP扩展开发》-协程-重构Channel（二）.md)

[71、自定义Socket对象](./docs/《PHP扩展开发》-协程-自定义Socket对象.md)

[72、修复一些bug（十一）](./docs/《PHP扩展开发》-协程-修复一些bug（十一）.md)

[73、重构Server](./docs/《PHP扩展开发》-协程-重构Server.md)

### 第四阶段 Hook

[74、hook原来的sleep](./docs/《PHP扩展开发》-协程-hook原来的sleep.md)

[75、stream_socket_server源码分析](./docs/《PHP扩展开发》-协程-stream_socket_server源码分析.md)

[76、替换php_stream_generic_socket_factory](./docs/《PHP扩展开发》-协程-替换php_stream_generic_socket_factory.md)

[77、如何bind和listen](./docs/《PHP扩展开发》-协程-如何bind和listen.md)
