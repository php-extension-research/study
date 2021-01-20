# 实现coroutine::Socket::bind和listen

因为`bind`和`listen`这两个函数并不会阻塞，所以这两个函数实现起来非常的简单，就是对原来的函数的一个封装。我们在文件`src/coroutine/socket.cc`里面编写代码：

```cpp
int Socket::bind(int type, char *host, int port)
{
    return stSocket_bind(sockfd, type, host, port);
}

int Socket::listen()
{
    return stSocket_listen(sockfd);
}
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

`OK`，符合预期。

[下一篇：协程化Socket::accept](./《PHP扩展开发》-协程-协程化Socket::accept.md)

