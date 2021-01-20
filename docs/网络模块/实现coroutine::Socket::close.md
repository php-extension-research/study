# 实现coroutine::Socket::close

因为`close`这个函数并不会阻塞，所以这个函数实现起来非常的简单，就是对原来的函数的一个封装。我们在文件`src/coroutine/socket.cc`里面编写代码：

```cpp
int Socket::close()
{
    return stSocket_close(sockfd);
}
```

因为我们之前并没有对`close`函数进行封装，所以我们需要去实现`stSocket_close`函数。我们现在文件`include/socket.h`里面进行声明：

```cpp
int stSocket_close(int fd);
```

然后在文件`src/socket.cc`里面进行实现：

```cpp
int stSocket_close(int fd)
{
    int ret;

    ret = close(fd);
    if (ret < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    return ret;
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

[下一篇：协程化服务器（一）](./《PHP扩展开发》-协程-协程化服务器（一）.md)

