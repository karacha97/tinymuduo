# tinymuduo网络事件库


基于muduo事件库编写，实现了muduo库的基本功能：
1. 采用one loop per thread的处理方式，并通过wakeupfd唤醒阻塞的工作线程；
2. 采用C++11编写线程、互斥锁等模块，取代原本的boost库和pthread线程库，整体代码更简洁；
3. 设计环形缓冲区，更高效地读写文件；
4. 编写了基本的Http服务器，并进行了压力测试，测得实验虚拟机下可达到上万QPS；

## 反应式服务器 Reactor模式
采用multiple reactor模式，可以有效提高并发连接数

![/figure/mreactor.svg](https://github.com/karacha97/tinymuduo/blob/main/figure/mreactor.svg #pic_center) 

## 测试结果

测试环境：
- ubuntu 20.04.1
- 内存4GB
- CPU 4核

![/figure/testresult.png](https://github.com/karacha97/tinymuduo/blob/main/figure/testresult.png  #pic_center)
