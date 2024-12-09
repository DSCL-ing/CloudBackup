// #ifndef SINK_HPP
// #define SINK_HPP
#pragma once

#include"util.hpp"
#include"message.hpp"
#include<memory>
#include<cassert>
#include<fstream>
#include<sstream>


//日志落地模块 -- 指定输出位置

namespace xlog{
  class LogSink{
    public:
      using s_ptr = std::shared_ptr<xlog::LogSink>;
      LogSink() {}
      virtual ~LogSink() {}
      virtual void log(const char *data, size_t len) = 0;
      //信息数据与长度
  };

  class StdoutSink:public LogSink{
    public:
      StdoutSink()=default;
      void log(const char *data,size_t len)override{
        std::cout.write(data,len);
      }
  };
  class FileSink : public LogSink{
    public:
      FileSink(const std::string& pathname)
        :_pathname(pathname)
      {
        //保证目录存在
        util::FileUtil::createDirectory(util::FileUtil::getPath(_pathname));
        //取得文件句柄 : 二进制写+追加
        _ofs.open(_pathname,std::ios::binary|std::ios::app);
        assert(_ofs.is_open());
      }
      void log(const char *data,size_t len)override{
        _ofs.write(data,len);
        if(!_ofs.good()){
          std::cout<<"FileSinK:日志文件输出失败!"<<"\n";
          abort();
        }
      }

      private:
      std::string _pathname; //文件路径
      std::ofstream _ofs;//文件句柄
  };

  class RollBySizeSink:public LogSink{
    public:
      RollBySizeSink(std::string basename,size_t max_fsize)
      :_basename(basename),_max_fsize(max_fsize),_name_count(0)
      {
        //保存目录存在
        util::FileUtil::createDirectory(util::FileUtil::getPath(_basename));
        //构建文件名
        std::string filename = createNewFileName();
        //获取文件句柄
        _ofs.open(filename,std::ios::binary|std::ios::app);
        if(!_ofs.is_open()){
          std::cout<<"RollBySizeSink: 打开文件失败!"<<"\n";
          abort();
        }
      }
      void log(const char *data ,size_t len) override{
        if(_cur_fsize>=_max_fsize){
          //每次新建文件时需要清零,否则在1s内会一直创建文件,且创建的文件是相同的,即1s内使用的依旧是旧文件.
          _cur_fsize = 0;
          //关闭旧的文件句柄
          _ofs.close();
          //构建文件名
          std::string filename = createNewFileName();
          //获取文件句柄
          _ofs.open(filename,std::ios::binary|std::ios::app);
          if(!_ofs.is_open()){
            std::cout<<"RollBySizeSink::xlog: 打开文件失败!"<<"\n";
            abort();
          }
        }
        _ofs.write(data,len);
        _cur_fsize+=len;
      }

    private:
      std::string createNewFileName(){
        //根据当前时间构建 // ./logs/base-20230102030507.log
        time_t timestamp = util::DateUtil::getCurTime();
        struct tm tm; //或者命名lt:localtime
        localtime_r(&timestamp,&tm);

        std::stringstream ss;//自动类型推导特性
        ss<<_basename;
        ss<<tm.tm_year+1900;
        ss<<tm.tm_mon+1;
        ss<<tm.tm_mday;
        ss<<tm.tm_hour;
        ss<<tm.tm_min;
        ss<<tm.tm_sec;
        ss<<"-";
        ss<<_name_count++;
        ss<<".log";

        return ss.str();
      }

    private:
      std::string _basename; //用户自定义文件名前缀
      std::ofstream _ofs;      //文件句柄
      size_t _max_fsize;      //用户定义最大存储大小
      size_t _cur_fsize;      //当前已写入大小
      size_t _name_count;     //命名编号:防止时间过短时命名相同
  };

  //类简单工厂 -- 根据参数返回对应的产品
  //因为产品的构造参数不同,传参不易 + 简单工厂时增加产品(用户自定义落地方式)时需要修改源代码(破坏封装),
  //实现工厂方法模式代码量大,因此采用模板+可变参数包+简单工厂更优
  class SinkFactory{
    public:
      template<class SinkType,class ... Args>
        static LogSink::s_ptr create(Args&& ... args){
          return std::make_shared<SinkType>(std::forward<Args>(args)...); //完美转发+可变参数-- 常客:能够接收任意参数
        }
  };


}//namespace_log_END


// #endif
