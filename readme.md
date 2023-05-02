# Introduction

A hobby OS named "Mosix 3d".

Developed by a student of grade 9.

# Features

- x64 (long mode)
- SMP (multi-processor)
- APIC
- ACPI
- vfs (including mount & dev)
- Multitask (multi processes; without threads)
- Virtual memory management
- PE executable
- Almost completely self-written: **NO code from haribote**

# Build and run

## Environment

**Linux**-like systems as well as **WSLs** with
```
gcc x86_64-w64-mingw32-gcc nasm bochs
```
installed are recommended.

Can't run on windows due to different slashes and commands !!!

e.g. my environment:
```
Linux (hidden) 5.10.16.3-microsoft-standard-WSL2 #1 SMP Fri Apr 2 22:23:49 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
```

## Download

Download (or clone) this repository into a directory:
```bash
git clone https://gitee.com/xthoa/mosix-3d
```

## Build, run, debug

Run `make` or `make build` in /path/to/repo/ and 
this will generate `mosix.img` in this directory.

Run `make bochs` or `make bochs-debug` to run or debug 
in bochs. Same to qemu if installed.

Config can be found in /path/to/repo/makefile.

# Update history

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

v13 2023-2-7（仅虚拟机）
以内核模式dll实现加载驱动
附pci.drv作为测试，由init.exe启动，输出pci遍历信息

v14 2023-3-18
实现键盘驱动ps2kbd.drv并由init启动，取消pci输出
接受键盘输入，实时显示

v15 2023-3-25
实现通用buffer数据结构，并用以实现键盘设备文件
init通过vfs接口显示键盘内容
修复很多bug并增强debug设施

v16 2023-4-2
实现ide/ata硬盘驱动

v17 2023-4-5（真机测试不充分）
实现pty，增加vgatty和sh0作为测试

v18 2023-4-8（从此版本开始，不再做真机测试）
完善部分内核代码；
为sh0添加一些命令，并可以通过全路径运行应用程序；
附sprintf动态库；
附mem.exe显示内存使用情况

v19 2023-4-9
完善mount和path部分实现；
完善pwd和cd命令；
更新mem.exe表现

v20 2023-4-15
在内核中实现cwd

v21 2023-4-16（22）
实现heap
实现内核栈，解决栈内存泄露问题

v21a 2023-4-22
实现命令行参数传递功能
更新mem.exe并带上参数解析功能
调整了各系统程序的起始路径

v22 2023-4-27
实现getdents接口和ls.exe

v22a 2023-4-28
设计了大部分兼容linux的错误码机制

v23 2023-5-2
实现mbr分区表驱动，自动对所有块设备检测分区
完善ls和sh0
