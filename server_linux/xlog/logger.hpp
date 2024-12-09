// #ifndef LOGGER_HPP
// #define LOGGER_HPP

#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <string>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdarg>
#include "format.hpp"
#include "sink.hpp"
#include "level.hpp"
#include "looper.hpp"
#include<unordered_map>

namespace xlog
{

  /*
    功能整合
    向外提供接口,完成不同等级的日志输出

    不同等级日志落地功能


    1.日志优先级
    2.日志器名(唯一标识)
      消息对象
      不定参解析 (解析可变参数列表和输出格式,实现和printf相同的方式)
    3.格式化
    4.日志落地
      互斥输出(同步/异步)
      互斥锁
      落地模块数组(多落地位置)

   */

  // 日志器基类
  class Logger
  {
  public:
    using s_ptr = std::shared_ptr<xlog::Logger>;
    Logger(const std::string &logger_name,
           LogLevel::Value level,
           Formatter::s_ptr& formatter,
           std::vector<LogSink::s_ptr> &sinks)
        : _logger_name(logger_name), _limit_level(level), _formatter_sp(formatter), _sinks(sinks.begin(), sinks.end())
    {}

    const std::string& name(){
      return _logger_name;
    }

    // 构造对应等级的日志消息对象并格式化成日志消息字符串,然后进行落地输出
    //  是否满足等级 -> 解析不定参 -> serialize(封装,可略){ LogMsg msg -> formatter(pattern).format(msg)-> data -> sink }
    void debug(const std::string &file, size_t line, const char *fmt, ...)
    {
      // 判断等级
      if (_limit_level > LogLevel::Value::DEBUG)
      {
        return;
      }

      // 解析不定参
      va_list arg; // al,arg,ap,arg_ptr,char*
      va_start(arg, fmt);
      char *buf;
      vasprintf(&buf, fmt, arg); // 解析不定参,转换成字符串 --GNU,自动计算并malloc
      va_end(arg);
      serialize(LogLevel::Value::DEBUG, file, line, buf);
      free(buf);
    }
    
    void info(const std::string &file, size_t line, const char *fmt, ...)
    {
      // 判断等级
      if (_limit_level > LogLevel::Value::INFO)
      {
        return;
      }

      // 解析不定参
      va_list arg; // al,arg,ap,arg_ptr,char*
      va_start(arg, fmt);
      char *buf;
      int len = vasprintf(&buf, fmt, arg); // 解析不定参,转换成字符串 --GNU,自动计算并malloc
      if(len<0){
        std::cout<<"解析的字符串:"<<buf<<std::endl;
        assert(false);
      }
      va_end(arg);
      serialize(LogLevel::Value::INFO, file, line, buf);
      free(buf);
    }
    
    void warn(const std::string &file, size_t line, const char *fmt, ...)
    {
      // 判断等级
      if (_limit_level > LogLevel::Value::WARN)
      {
        return;
      }
      // 解析不定参
      va_list arg; // al,arg,ap,arg_ptr,char*
      va_start(arg, fmt);
      char *buf;
      vasprintf(&buf, fmt, arg); // 解析不定参,转换成字符串 --GNU,自动计算并malloc
      va_end(arg);
      serialize(LogLevel::Value::WARN, file, line, buf);
      free(buf);
    }
    
    void error(const std::string &file, size_t line, const char *fmt, ...)
    {
      // 判断等级
      if (_limit_level > LogLevel::Value::ERROR)
      {
        return;
      }

      // 解析不定参
      va_list arg; // al,arg,ap,arg_ptr,char*
      va_start(arg, fmt);
      char *buf;
      vasprintf(&buf, fmt, arg); // 解析不定参,转换成字符串 --GNU,自动计算并malloc
      va_end(arg);
      serialize(LogLevel::Value::ERROR, file, line, buf);
      free(buf);
    }
    void fatal(const std::string &file, size_t line, const char *fmt, ...)
    {
      // 判断等级
      if (_limit_level > LogLevel::Value::FATAL)
      {
        return;
      }

      // 解析不定参
      va_list arg; // al,arg,ap,arg_ptr,char*
      va_start(arg, fmt);
      char *buf;
      vasprintf(&buf, fmt, arg); // 解析不定参,转换成字符串 --GNU,自动计算并malloc
      va_end(arg);
      serialize(LogLevel::Value::FATAL, file, line, buf);
      free(buf);
    }

  protected:
    void serialize(LogLevel::Value level, const std::string &file, size_t line, const std::string &buf)
    {
      // 构造消息对象
      LogMsg msg(level, file, line, _logger_name, buf);

      // 格式化
      std::string str = _formatter_sp->format(msg);

      // 日志:w
      log(str.c_str(), str.size());
    }
    virtual void log(const char *data, size_t len) = 0;

  protected:
    std::string _logger_name;
    std::atomic<LogLevel::Value> _limit_level; // 枚举成员本质属整型类 -- 多线程输出日志,频繁访问,竞态
    Formatter::s_ptr _formatter_sp;
    std::vector<LogSink::s_ptr> _sinks;
    std::mutex _mutex; // 防止出现竞态条件

  }; // class logger  __END__

  
 
//与任务同步进行
  class SyncLogger : public Logger
  {
  public:
    SyncLogger(const std::string &logger_name,
               LogLevel::Value level,
               Formatter::s_ptr& formatter,
               std::vector<LogSink::s_ptr> &sinks)
        : Logger(logger_name, level, formatter, sinks)
    {
    }

  protected:
    void log(const char *data, size_t len) override
    {
      std::unique_lock<std::mutex> lock(_mutex);
      // 日志落地
      for (auto &sink : _sinks)
      {
        sink->log(data, len);
      }
    }
  };



// logger_name level -> fmt -> sinks  //消息 - 格式化 - 落地
  class AsyncLogger : public Logger {
    public:
      AsyncLogger(const std::string& logger_name,
                  LogLevel::Value level,
                  Formatter::s_ptr& formatter,
                  std::vector<LogSink::s_ptr>& sinks,
                  AsyncType asynctype = AsyncType::ASYNC_SAFE)
          : Logger(logger_name, level, formatter, sinks),_looper(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::reallog,this,std::placeholders::_1),asynctype))
      {}

      //写到缓冲区中
    void log(const char *data, size_t len) override{
      _looper->push(data,len);
    }
    
    //实际日志输出
    void reallog(Buffer& buf){
      // std::unique_lock<std::mutex> lock(_mutex); //不需要锁,异步线程只有一个,是串行的
      if(_sinks.empty()){ return ; }
      for (auto &sink : _sinks) {
        sink->log(buf.begin(),buf.readAbleSize());
      }
    }
    
    private:
    AsyncLooper::s_ptr _looper;

  };


  enum class LoggerType
  {
    LOGGER_SYNC,
    LOGGER_ASYNC
  };

  // 建造者模式: 简化构造流程
  // 有两种日志器对象,分别是同步和异步日志器,直接构造logger太繁琐,使用建造者模式,简化构造流程
  // 抽象logger建造者基类,派生出同步模式与异步模式建造者


  class LoggerBuilder
  {
    public:
      LoggerBuilder()
        //default config
        : _asynctype(AsyncType::ASYNC_SAFE),_limit_level(LogLevel::Value::DEBUG), _logger_type(LoggerType::LOGGER_SYNC)
      { }

      //必需
      void buildLoggerName(const std::string &name) { _logger_name = name; }

      void buildEnableUnsafeAsync(){_asynctype = AsyncType::ASYNC_UNSAFE;}
      void buildLoggerType(LoggerType logger_type ){_logger_type = logger_type;}
      void buildLoggerLevel(LogLevel::Value level ) { _limit_level = level; }

      void buildFormatter(const std::string &pattern )
      {
        _formatter_sp = std::make_shared<Formatter>(pattern);
      }

      template <class SinkType, class... Args>
        void buildSink(Args &&...args)
        {
          LogSink::s_ptr sink_sp = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
          _sinks.push_back(sink_sp);
        }

      virtual Logger::s_ptr build() = 0;

    protected:
      AsyncType _asynctype;
      std::atomic<LogLevel::Value> _limit_level;
      LoggerType _logger_type;
      std::string _logger_name;
      Formatter::s_ptr _formatter_sp;
      std::vector<LogSink::s_ptr> _sinks; // 优化:使用set,保证唯一
  };

  class LocalLoggerBuilder : public LoggerBuilder
  {
    public:
      Logger::s_ptr build() override
      {
        assert(!_logger_name.empty());
        if (_formatter_sp.get() == nullptr) //传入格式为空
        {
          _formatter_sp = std::make_shared<Formatter>();
        }
        if(_logger_name.empty()){
          std::cout<<"logger_name is empty"<<std::endl;
        }
        if (_sinks.empty())
        {
          buildSink<StdoutSink>(); // 默认为标准输出
        }
        if (_logger_type == LoggerType::LOGGER_ASYNC)
        {
          return std::make_shared<AsyncLogger>(_logger_name,_limit_level,_formatter_sp,_sinks,_asynctype);
        }
        return std::make_shared<SyncLogger>(_logger_name, _limit_level, _formatter_sp, _sinks);
      }
  };


/*
  日志器管理器
  单例--全局唯一
  用map管理日志
  map互斥
  _root_logger //原始日志器
    
  添加日志器 map.insert
  日志器是否已管理exists -- , map.find
  获取日志器 map[loggername];
  获取默认日志器 --- 方便用户
 
  //日志器几乎不删除
 */

  class LoggerManager{
    public:
      static LoggerManager& getInstance(){
        static LoggerManager _instance;//只会初始化一次,线程安全
        return _instance; 
      }
      void addLogger(Logger::s_ptr& logger){ //自动获取日志器名
        if(hasLogger(logger->name()))
        std::lock_guard<std::mutex> lg(_mutex);
        _loggers.insert(std::make_pair(logger->name(),logger));
      }

      bool hasLogger(const std::string& name){
        auto it = _loggers.find(name);
        return it==_loggers.end()? false:true; 
      }

      Logger::s_ptr getLogger(const std::string&name){
        return hasLogger(name)? _loggers[name] :  nullptr;
      }
      
      Logger::s_ptr rootLogger(){
        return _root_logger;
      }

    private:
      LoggerManager(){
        //emmmmm....  
        std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder()); //先创建局部logger,后全局
        builder->buildLoggerName("root");
        _root_logger = builder->build();
        addLogger(_root_logger);
      };

    private:
      std::unordered_map<std::string,Logger::s_ptr> _loggers;
      std::mutex _mutex;
      Logger::s_ptr _root_logger;
  }; //class LoggerManager END


  //全局日志器建造者,降低用户使用成本,建造日志器直接为全局对象;
//为了实现全局日志器建造者,代价很大 -- 耦合度很高,交叉引用
  class GlobalLoggerBuilder : public LoggerBuilder
  {
    public:
      Logger::s_ptr build() override
      {
        assert(!_logger_name.empty());
        if (_formatter_sp.get() == nullptr) //传入格式为空
        {
          _formatter_sp = std::make_shared<Formatter>();
        }
        if(_logger_name.empty()){
          std::cout<<"logger_name is empty"<<std::endl;
        }
        if (_sinks.empty())
        {
          buildSink<StdoutSink>(); // 默认为标准输出
        }
        Logger::s_ptr logger;
        if (_logger_type == LoggerType::LOGGER_ASYNC)
        {
          logger =  std::make_shared<AsyncLogger>(_logger_name,_limit_level,_formatter_sp,_sinks,_asynctype);
        }
        else {
          logger =  std::make_shared<SyncLogger>(_logger_name, _limit_level, _formatter_sp, _sinks);
        }
        xlog::LoggerManager::getInstance().addLogger(logger);
        return logger;
      }
  }; // class GlobalLoggerBuilder END



} // namespace_log_END

// #endif
