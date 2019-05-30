---
title: 《PHP扩展开发》--协程(规范)
date: 2019-05-30 18:43:34
tags:
- PHP
- PHP扩展
- Swoole
---

在开始开发之前，很有必要先说一下开发的规范问题。

1、我们的`PHP`扩展函数的声明都放在文件`php_study.h`里面。例如：

```c++
PHP_FUNCTION(test);
```

2、只在项目根目录下的`study_*.cc`文件里面使用`Zend API`，不在`src`目录的代码里面使用`Zend API`。

（待补充）
