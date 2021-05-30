# 一些软硬件算法的模拟与实现

## TAPSA

单个处理器基于表的优先级计算与排序。

输入文件格式

```c
5 // 5个任务
1 0 // 任务1执行时间为1，释放时间为0
1 3 // 任务2
2 0 // 任务3
1 0 // 任务4
1 0 // 任务5
1 4 // 依赖关系：1->4
1 5 // 依赖关系：1->5
2 4 // 依赖关系: 2->4
2 5 // 依赖关系: 2->5
3 5 // 依赖关系: 3->5
```

## MUPSA

在TAPSA计算得到的优先级基础上，对多个处理器进行优先级调度的模拟。本实现支持非抢占式和抢占式两种配置。

输入文件格式

```c
0   // 0表示非抢占式，1表示抢占式
5 2 // 5个任务，2个cpu
1 0 // 任务1执行时间为1，释放时间为0
1 3 // 任务2
2 0 // 任务3
1 0 // 任务4
1 0 // 任务5
1 4 // 依赖关系：1->4
1 5 // 依赖关系：1->5
2 4 // 依赖关系: 2->4
2 5 // 依赖关系: 2->5
3 5 // 依赖关系: 3->5
```

## MMM 多模块划分算法

> 这里的模块也即簇。

执行命令

```shell
$ g++ mmm.c
$ ./a.out [input file] [0/1/2]
```

最后一个参数用于选择两模块是否能合并的算法。

+ 0: 使用单链接
+ 1: 使用全链接
+ 2: 使用均链接

输入文件格式

```c
10 3 4 // 总任务数，最终划分模块数，单个模块内任务数上限
1 2 1 // 任务1，2间通信代价为1
1 7 4
1 9 2
2 3 3
3 8 2
3 10 1
4 6 2
4 8 5
4 9 6
5 8 3
5 9 7
5 10 8
6 7 1
```

输出为三元组`<c, k, K>`，分别表示

+ 通信代价阈值
+ 划分后模块的数量
+ 模块集合
