# 这是什么？

这是使用 ucontext + cpp 写的一个协程库

# 感谢

此项目是根据 https://github.com/xiaobing94/coroutine 修改完成。非常感谢原作者提供的文档和简明的代码

另外感谢 ntyCO，虽然我最终也没有根据那个项目做成项目，但是它的带我入了协程的门槛，让我知道学习协程需要的知识和方法

# 故障处理

代码重写期间碰到过一些问题，最后一个问题是 SIGBUS 的问题。此代码发生在 yield 的最后一行代码上，最终定位结果到库代码中。此问题的根本原因是协程栈太小了

倒数第二个问题忘了发生的结果，但是 gdb 调试时发现 coroutines_ 在 yield 之前一直有效，之后却突然失效，使用 `p coroutines_[0]` 得到的结果是内存由于权限问题（好像是）无法访问。之后调用 coroutines_[0].state 导致系统崩溃。此问题不知道到底怎么解决的。解决之后就变成了上面的那个问题。因此猜测也是栈太小的原因

# 说明

虽然我没仔细看 https://blog.csdn.net/liushengxi_root/article/details/85142236 但是此库也确实或多或少的帮到了我（maybe？）
