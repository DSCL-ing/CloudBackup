// #ifndef LOG_H
// #define LOG_H

#pragma once


#include"logger.hpp"

namespace xlog{

  //提供一些常用的简易全局(宏)函数,便于用户快速上手

  Logger::s_ptr getLogger(const std::string&name){
    return LoggerManager::getInstance().getLogger(name);
  }

  Logger::s_ptr rootLogger(){
    return LoggerManager::getInstance().rootLogger();
  }


/*宏不受命名空间约束*/
/*宏函数标识与括号间不能带空格*/

  //宏函数对日志器接口进行代理(代理模式)
  //#define debug(fmt,...) debug(__FILE__,__LINE__,fmt,##__VA_ARGS__)
  //#define info(fmt,...) info(__FILE__,__LINE__,fmt,##__VA_ARGS__)
  //#define warn(fmt,...) warn(__FILE__,__LINE__,fmt,##__VA_ARGS__)
  //#define error(fmt,...) error(__FILE__,__LINE__,fmt,##__VA_ARGS__)
  //#define fatal(fmt,...) fatal(__FILE__,__LINE__,fmt,##__VA_ARGS__)
  

  //提供使用默认日志器进行标准输出打印的全局宏函数
  //#define DEBUG(fmt,...) xlog::rootLogger()->debug(fmt,##__VA_ARGS__)
  //#define INFO(fmt,...) xlog::rootLogger()->info(fmt,##__VA_ARGS__)
  //#define WARN(fmt,...) xlog::rootLogger()->warn(fmt,##__VA_ARGS__)
  //#define ERROR(fmt,...) xlog::rootLogger()->error(fmt,##__VA_ARGS__)
  //#define FATAL(fmt,...) xlog::rootLogger()->fatal(fmt,##__VA_ARGS__)


}



// #endif
