// #ifndef LOOPER_HPP
// #define LOOPER_HPP

#pragma once

#include<iostream>
#include<thread>
#include<condition_variable>
#include<atomic>
#include<functional>
#include"buffer.hpp"

namespace xlog{
//异步工作器 looper:双缓冲循环
/*
    状态:启动 -- 停止 :bool _stop
    两个缓冲区:生产者-消费者:锁,条件变量
    写入(push) --- 输出(异步 + 回调) :异步线程与异步任务

    版本控制 safe & unsafe

*/

enum class AsyncType{
  ASYNC_SAFE, //安全状态,表示缓冲区满则阻塞,避免资源耗尽的风险
  ASYNC_UNSAFE//不保证安全,不考虑资源问题,无限扩容,用于测试极限性能
};
    
    
using Functor = std::function<void(Buffer&)>; //处理缓冲区的任务
    class AsyncLooper{
    public:
        using s_ptr= std::shared_ptr<xlog::AsyncLooper>;
        AsyncLooper(const Functor& callback,AsyncType looper_type = AsyncType::ASYNC_SAFE)
        :_looper_type(looper_type),_stop(false),
        _callback(callback),
        _thread(&AsyncLooper::threadEntry,this)
        {}
        
        ~AsyncLooper(){
          stop();
        }

        void stop(){
          _stop = true;
          _cond_con.notify_all();
           _thread.join(); 
        }
       
//使用future? 代码更优雅

        void push(const char* data, size_t len){

          //1.无线扩容--非安全(用于压力测试) 2.阻塞--安全
          std::unique_lock<std::mutex> lock(_mutex);
          {

            //只针对阻塞模式,写满就休眠,等待唤醒;能写入就唤醒消费者 --- 只有生产者知道有没有数据
            if(_looper_type == AsyncType::ASYNC_SAFE){
              _cond_pro.wait(lock,[&](){return len>_buf_pro.writeAbleSize()?false:true;}); //一行代码决定是否安全模式
            }
            // 性能: 输出很慢+写满阻塞时,性能影响严重; 输出速度>输入时,阻塞少,高性能. 


            //串行插入--线程安全
            _buf_pro.push(data,len);
            _cond_con.notify_all(); //保证是当前push线程,只唤醒一次;只有一个异步线程,只用于条件变量的锁
          }
        }

        //异步任务线程入口
        /*
           双缓冲异步任务逻辑:
           主线程插入数据,说明生产数据 --> 通知异步线程启动,取走数据(交换)进行处理(只有一个异步线程,通知到说明此时消费缓冲区为空,异步线程正在阻塞;通知不到说明此时异步线程正在工作)
           异步线程:启动时,检查生产者情况进行处理;之后callback处理数据时串行化(只有一个异步线程),处理完数据(一次必定全部处理完)后,循环检查进行条件判断,检查生产者buf的情况,确保在生产者通知后,异步线程没收到时直接进入休眠(错误逻辑),不满足再进入休眠,等待通知/唤醒

           线程循环检测:生产者缓冲为空时阻塞;不为空时,将生产者与消费者缓冲区置换;

           起始状态:启动时实例化线程对象,启动线程; 刚启动时,没有数据; 结束时,会处理完所有数据再结束

           条件: 进入时消费者buf一定没有数据 ---> 等生产者通知

*/


/*
_stop/_running变量用于控制起始状态
empty用于维护同步关系
除非说程序没有起始状态,就可以只使用empty

运行时+生产缓冲区为空时阻塞;    
运行时+生产缓冲区非空是继续;    
非运行时+生产缓冲区为空时退出;
非运行时+生产缓冲区非空时继续;

A:运行时
B:为空
C:继续
A B C
1 0 0  //阻塞
1 1 1
0 0 x  //出口 -- 
0 1 1
即!run||empty

条件分析:
  定义:
  _stop:s
  _pro.empty:e

  s&e   -break
  s&!e  -continue
  !s&e  -sleep
  !s&!e -continue

!_buf_pro.empty()||_stop;
  A  B  C
  0  0  1
  0  1  1
  1  0  0  //运行时+生产缓冲区为空时阻塞;
  1  1  1
 
 */

        //通过_stop控制线程任务的启停
        void threadEntry(){
          while(1){
            {
              std::unique_lock<std::mutex> lock(_mutex);
              //保证停止前输出完所有数据 -- 只要有数据就不停止

              if (_stop == true && _buf_pro.empty()) { break; }
              
              //运行时+生产缓冲区为空时阻塞;
              //_stop状态时,需要唤醒所有线程执行到被join,不然程序会休眠阻塞
              _cond_con.wait(lock,[&](){return !_buf_pro.empty()||_stop;}); //捕获this

              //走到这里,不为空,取走数据
              _buf_con.swap(_buf_pro);

              //通知生产者 --- 锁内,保证是当前线程,只唤醒一次
              _cond_pro.notify_all();
            }
            //2.数据处理,处理完毕后重置
            _callback(_buf_con); // 数据处理由外界负责,不加锁 --- 只有一个线程,即串行化,不需要保护
            _buf_con.reset();
          }
        }

    private:
        AsyncType _looper_type; //安全类型|非安全类型
        std::atomic<bool> _stop; //启停标记

        std::mutex _mutex; 
        std::condition_variable _cond_pro; //producer
        std::condition_variable _cond_con; //consumer


        Functor _callback; //输出任务
        std::thread _thread;    //异步输出任务线程

        Buffer _buf_pro; 
        Buffer _buf_con; //资源自动释放
    };

}

// #endif

