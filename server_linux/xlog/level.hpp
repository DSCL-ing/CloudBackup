// #ifndef LEVEL_HPP
// #define LEVEL_HPP

#pragma once

namespace xlog{
  class LogLevel{
    public:
      enum class Value{
        UNKNOW = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
      };

      //返回错误等级的C字符串
      static const char * toString(LogLevel::Value value){
        switch(value){
          case Value::DEBUG: return "DEBUG";
          case Value::INFO: return "INFO";
          case Value::WARN: return "WARN";
          case Value::ERROR: return "ERROR";
          case Value::FATAL: return "FATAL";
          case Value::OFF: return "OFF";
          default:
                           break;
        }
        return "UNKNOW";
      }
  };//CLASS_LogLevel__END 


} //namespace_log_END



// #endif


