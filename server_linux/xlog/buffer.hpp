// #ifndef BUFFER_HPP
// #define BUFFER_HPP


#pragma once

#include<iostream>
#include<vector>
#include<cassert>


//同步写日志过程(直接落地)可能写的比较慢,写入量多等,为了避免因写日志过程阻塞带来的影响,实现异步落地日志器
//异步:(间接落地)不让业务线程进行日志的实际落地操作,而是将日志消息放到缓冲区(一块指定的内存)中,让一个专门的异步线程去将该缓冲区中的数据进行处理
 
//异步优点:避免了同步日志可能因IO问题阻塞的情况
//异步缺点:理想情况下,没有同步日志快 


//日志一般只会用一个线程 -- 主要资源用于业务
//但也要保证线程安全 -- 

  //异步缓冲区实现路程:
  /*
  1. 队列 先进先出特性 --> STL队列:链式队列,输入输出过程频繁申请与释放内存,性能浪费严重. 生产消费竞争锁,效率低
  2. 环形队列 定长,循环使用 --> 生产消费模型:竞态行为不那么严重: 主要瓶颈:高并发场景中信号量为主要性能开销,信号量涉及内核态,频繁切换开销
  3. 双缓冲 交换---> 避免生产与消费频繁的锁冲突(串行化) 优点:只在生产满时或消费完时发生生产消费一次互斥; 输出: 一次性IO输出,减少IO次数,提高效率
    //高效 -- linux 网络IO中接收,发送双缓冲区...
  */

 /*
  缓冲区设计: 一个缓冲区负责输入,另一个缓冲区负责输出; 当输出完毕,初始化缓冲区,然后角色交换(输入缓冲区也为空时,不交换);
  存储数据类型: 只存储格式化好的字符串 -- 避免Msg对象频繁创建带来的性能开销
  数据结构: std::vector -- 
  读指针rseek: 指向当前读取的位置,读到写指针位置时,表明读完,交换
  写指针wseek: 指向当前写入的位置,写到读指针位置时,表示写满,交换

  提供的缓冲区管理操作:
  - 读写操作
    1.向缓冲区中写入数据 push() 
        .写指针偏移moveWrite    不需要写地址,push内部实现   .可写数据长度 writeAbleSize()
    2.从缓冲区中读入数据 (虚:读数据会再次发生拷贝,原位置不需要保留 --> 不读,直接移动 --> 几乎与同步日志效率一样)
        .读指针偏移moveReader   .获取可读数据起始地址begin  .获取可读数据长度readAbleSize() 
  - 初始化 reset()
  - 交换(地址交换) 
  - empty()
 
logic:
    buffer( rseek, wseek, _vector<字节流>)
        写入push --- len>writeAbleSize()? 扩容,写入:写入 --- moveWseek(单位)
            扩容ensureEnouthSize
        读出 --->  stream.write() : ReadAbleSize + moveRseek
            



 */

namespace xlog{


  /* threshold: 阈值 */
  #define DEFAULT_BUFFER_SIZE 1*1024*1024  //1M
  #define THRESHOLD_BUFFER_SIZE 10*1024*1024 //5M -- 阈值:阈值以下,翻倍增长;阈值以上 线性增长
  #define INCREMENT_BUFFER_SIZE 1*1024*1024 //1M -- 增量大小:线性增长增量, --- 
                                            
                                           
    class Buffer{
    public:
        Buffer()
        :_buffer(DEFAULT_BUFFER_SIZE),_rindex(0),_windex(0)
        {} 

        void push(const char* data,size_t len){
          //空间不足: 1.阻塞(实际业务,空间有限),返回false  2.扩容(性能测试)
          //阻塞能够交由外部控制(writeAbleSize),因此不需要在buffer中考虑,buffer只管提供基本的接口
          //外界能够根据writeAbleSize来判断是否进行插入-- 实现控制定容或扩容
          //代码实现更加高效 -- 解耦合
          
          // if(len>writeAbleSize()) return; //定容 --- 
          
          ensureEnoughSize(len); //扩容
          std::copy(data,data+len,&_buffer[_windex]);
          moveWrite(len);
        }

        //返回可写大小
        size_t writeAbleSize() { return _buffer.size() - _windex; }

        //返回可读大小
        size_t readAbleSize(){ return _windex - _rindex; }
          // IF: _widx == _ridx ==0 ; ---> push 1; _widx+=1;  
          // THEN: _widx-_ridx==1-0=1 == readAbleSize;
       
        //移动读指针
        void moveReader(size_t len){
          assert(len<=readAbleSize()); 
          _rindex += len;
        }

        //移动写指针
        void moveWrite(size_t len) {
          assert(len<=writeAbleSize()); //针对定容情况
          _windex += len;
        }

        //返回可读数据的起始地址
        const char* begin(){ return &_buffer[_rindex]; }


        void swap(Buffer &buffer){
          _buffer.swap(buffer._buffer);
          std::swap(buffer._rindex,_rindex);
          std::swap(buffer._windex,_windex);
        }

        //判空
        bool empty() { return _windex - _rindex == 0 ? true : false; } // 相等为空
        
        //初始化 : 交换前置空
        void reset(){
          _rindex = _windex = 0;
        }

      private:
        //扩容 -- 确保有足够空间
        void ensureEnoughSize(size_t len){ //简单复现 -- linux 网络IO 拥塞控制
          if (len <= writeAbleSize()) return;
          size_t new_capacity = 0;
          if (_buffer.size() < THRESHOLD_BUFFER_SIZE) {
            new_capacity = _buffer.size() * 2 + len;
          }
          else {
            new_capacity = _buffer.size() + INCREMENT_BUFFER_SIZE + len;
          }
          _buffer.resize(new_capacity);
          //+len确保扩容的大小足以容纳len; 或者不加len,使用循环扩容直到容纳len;
        }
        

      private:
       std::vector<char> _buffer; //字节流vector -- 
       size_t _rindex;            //读指针
       size_t _windex;            //写指针
    };

}//namespace_xlog__END



// #endif
