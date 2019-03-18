# Intro
- main_watchpoint_child_from_parent.cpp : 在parent中fork一个子进程, 然后监测子进程对某个内存地址的修改. 该文件主要是仿造android/bionic/tests/sys_ptrace_test.cpp写的.
- main_watchpoint_parent_from_child.cpp : 在parent中fork一个子进程, 然后在子进程中监测父进程对某个内存地址的修改. 该文件可用来集成到具体的项目代码中来跟踪踩内存问题.

# Build
- ln -sf main_watchpoint_xxx.cpp main.cpp
- mmm path-to-project

# Usage
- 针对main_watchpoint_parent_from_child.cpp, 当父进程修改某个内存内容时, 内核会往父进程发送一个SIGTRAP信号. Android的debuggerd机制会捕获该信号, 然后打印进程的调用栈. 可在logcat中搜索SIGTRAP.
- 但针对main_watchpoint_child_from_parent.cpp, 我们在logcat中看不到SIGTRAP, 因此我们在代码中用了PTRACE_TRACEME, 这会导致所有发给子进程的信号被转发给父进程, 所以SIGTRAP也会被转给父进程. 而且可能由于某种原因并没有触发debuggerd捕获该信号.

# Known Issue
在我们的demo中, 每次都能捕获到内存修改操作.
但悲剧的是, 在实际项目代码中, 好像无法探测到内存被修改, 原因暂时不知, 只能后续在关注.