# 开发规范

在开始开发之前，很有必要先说一下开发的规范问题。

1、全局`PHP`扩展函数的声明都放在文件`php_study.h`里面。例如：

```c++
PHP_FUNCTION(test);
```

然后全局`PHP`扩展函数的实现放在`study_*.cc`里面。

2、类的方法声明放在相应的`study_*.cc`文件里面，并且声明为`static`类型。例如：

```c++
static PHP_METHOD(study_coroutine, test);
```

3、扩展函数以及扩展方法的参数声明放在`study_*.cc`、`study.cc`里面。

4、只在项目根目录下的`study_*.cc`文件里面使用`Zend API`，不在`src`目录的代码里面使用`Zend API`。

5、用到的标准库函数都在`include/study.h`里面引入。

6、为了防止重复引入头文件，增加如下条件编译。例如，在`study.h`头文件里面写入：

```c++
#ifndef STUDY_H_
#define STUDY_H_


#endif /* STUDY_H_ */
```

注意宏的命名规范。

7、只在`php_study.h`头文件里面引入`PHP`相关的头文件。`study_*.cc`只引入`php_study.h`文件，不去引入`include/study.h`文件。`study.h`文件是被`php_study.h`引入的。

（待补充）

