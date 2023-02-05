# Introduction

Mosix 3d, hobby os, grade9.
x64, smp, acpi, vfs.
No haribote.

# Versions

v1 2022-11-1
复制3c并真机测试通过

v2 2022-11-2（无法通过编译）
完全实现mount

v3 2022-11-2
实现initfs（即archive）
可以通过接口读取archive中文件内容并显示

v4 2022-11-5
makefile添加头文件依赖自动生成
并解决了一些依赖滞后问题
添加了fd（文件描述符）和相应接口
修复了很多bug

v5 2023-1-14（仅bochs）
改进进程管理和调度模块
可以运行新进程、退出进程，实现完整的进程生命周期
修复了一些bug

v6 2023-1-15
完善mutex和create_process
实现wait_process

v7 2023-1-17
可加载运行64位PE可执行文件，预留32位和elf文件代码框架
修复一些高危bug
系统初始化完成后启动init.exe，输出Hello, init!

v8 2023-1-23（仅虚拟机）
整合进程、互斥量对象为dispatcher对象并实现wait_dispatcher
完善一些陈旧代码

v9 2023-1-30（仅虚拟机）
实现handle和相应接口；在进程退出时回收大部分资源
在运行init.exe前后显示内存使用情况

v10 2023-2-1（仅虚拟机）
实现MessageList；用irql概念改善调度代码
init.exe进行msglist测试

v11 2023-2-1（仅虚拟机）
实现process reaper，修复一些bug

v12 2023-2-5（仅虚拟机）
可加载pe格式dll动态库，init.exe输出相应测试信息
