# 开发规范

[仓库地址](https://github.com/php-extension-research/study)

在开始开发之前，很有必要先说一下开发的规范问题。

1、全局`PHP`扩展函数的声明都放在文件`php_study.h`里面。例如：

```cpp
PHP_FUNCTION(test);
```

2、全局`PHP`扩展函数以及扩展方法的参数声明放在`study_*.cc`、`study.cc`里面。

3、没有别名的全局`PHP`扩展函数的实现放在`study.cc`里面，有别名的全局`PHP`扩展函数的实现放在对应的`study_*.cc`里面。

例如，`study_coroutine_create`的别名是`Study\Coroutine::Create`。

4、扩展方法声明放在相应的`study_*.cc`文件里面，并且声明为`static`类型。例如：

```cpp
static PHP_METHOD(study_coroutine, test);
```

5、头文件的引入关系如下：

```
study_*.cc 引入 对应的 study_*.h
study_*.h 引入 php_study.h和需要的include/*.h
study.cc 引入 php_study.h
php_study.h 引入 php内核提供的头文件
php_study.h 引入 include/study.h
include/study.h 引入 标准库函数
src/*.c 引入 对应的/include/*.h
include/*.h 按需引入 其他的include/*.h
```

6、只在项目根目录下的`study_*.cc`文件里面使用`Zend API`，不在`src`目录的代码里面使用`Zend API`。

7、为了防止重复引入头文件，增加如下条件编译。例如，在`study.h`头文件里面写入：

```cpp
#ifndef STUDY_H_
#define STUDY_H_


#endif /* STUDY_H_ */
```

注意宏的命名规范。

（待补充）

[下一篇：整理文件](《PHP扩展开发》-协程-整理文件.md)