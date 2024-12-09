// #ifndef MESSAGE_HPP
// #define MESSAGE_HPP


#pragma once

/*
  日志消息类,存储日志中间信息
  1. 日志的输出时间   定位时间
  2. 日志等级         日志过滤
  3. 源文件名称       定位文件
  4. 源代码行号       定位行号
  5. 线程ID           定位线程
  6. 日志主体消息     载荷
  7. 日志器名称       区分多日志器,支持同时使用

Exam:
[2000-01-12 00:32:46][root][12345678][main.cc:99][FATAL] 创建套接字失败...
*/

#include<iostream>
#include<string>
#include<thread>
#include"level.hpp"
#include"util.hpp"

namespace xlog{

  struct LogMsg{

    //日志优先级,文件,行号,日志器,数据
    LogMsg(LogLevel::Value level,
        const std::string filename,
        size_t line,
        const std::string& loggername,
        const std::string& msg)
      :_time(util::DateUtil::getCurTime()),
      _loggername(loggername),
      _tid(std::this_thread::get_id()),
      _filename(filename),
      _line(line),
      _level(level),
      _payload(msg)
    { }

    time_t _time;
    std::string _loggername;
    std::thread::id _tid;
    std::string _filename;
    size_t _line;
    LogLevel::Value _level;
    std::string _payload; //message
  };
} //namespace_log_END


// #endif
