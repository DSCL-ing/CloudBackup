// #ifndef FORMAT_H
// #define FORMAT_H

#pragma once

#include<iostream>
#include<string>
#include<sstream>
#include<ctime>
#include<vector>
#include<memory>
#include<cassert>

#include<unistd.h>


#include"util.hpp"
#include"message.hpp"
#include"level.hpp"

/*
  日志格式化模块
  : 对日志消息进行格式化,组织成指定格式的字符串

pattern成员：保存⽇志输出的格式字符串。 
 %d ⽇期 
 %t 线程id 
 %p ⽇志优先级/级别     -- DEBUG,ERROR...
 %c ⽇志器名称category  -- [root]
 %f ⽂件名 
 %l ⾏号 
 %m ⽇志消息 
 %T 缩进 
 %n 换⾏ 

格式化字符串分解:
Exam: 
[%d{%H:%M:%S}][%f:%l]%p%m%n
  1. 非格式化字符子项 [
  2. 格式化字符子项   %p
  3. 日期子项         {%H:%M:%S}
  4. 非格式化字符子项 ]
  5. 非格式化字符子项 [
  6. 文件名子项       %f 
  7. 非格式化字符子项 :
  8. 行号子项         %l
  9. 非格式化字符子项 ]
  10.消息主体子项     %m
  11.换行子项         %n

*/


//核心技术:类型擦除
/*
定义数组,类型为基类(格式),用于存储派生类(不同的格式)
解析格式规则,根据不同格式规则构造各个派生类型,存入基类数组中,实现统一的格式化方式
*/




namespace xlog{
  class FormatItem{
    public:
      virtual void format(std::ostream& out,const LogMsg& msg) = 0;
  };

  class TimeFormatItem :public FormatItem{
    public:
      TimeFormatItem(const std::string& format):_time_fmt(format){ }
      void format(std::ostream &out,const LogMsg& msg) override{
        struct tm t;
        localtime_r(&msg._time,&t);
        char tmp[32] = {0};
        strftime(tmp,31,_time_fmt.c_str(),&t);
        out<<tmp;
      }
    private:
      std::string _time_fmt;
  };

  class TidFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
          out<<msg._tid;    
      }
  };


  //priority level = PRI
  class LevelFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
       	out<<LogLevel::toString(msg._level);
      }
  };

  //LoggerName
  class LoggerFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        out<<msg._loggername;  
      }
  };

  //FileName
  class FileFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        out<<msg._filename;
      }
  };
  class LineFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        out<<msg._line; 
      }
  };
  class MsgFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        out<<msg._payload; 
      }
  };

  //NewLine
  class NLineFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        (void)msg;
        out<<"\n";
      }
  };
  class TabFormatItem:public FormatItem{
    public:
      void format(std::ostream &out,const LogMsg& msg){
        (void)msg;
        out<<"\t"; 
      }
  };

  class OtherFormatItem:public FormatItem{
    public:
      OtherFormatItem(const std::string& str):_str(str){ }
      void format(std::ostream &out,const LogMsg&msg){
        (void)msg;
        out<<_str;
      }
    private:
      std::string _str;
  };


  //格式化器 
  class Formatter{

    /*
      _pattern -> parsePattern() -> createItem() -> _items -> format() -> OUT:Msg
     */
    
    public:
      using s_ptr = std::shared_ptr<xlog::Formatter>;
      Formatter(const std::string& pattern = "[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n")
        :_pattern(pattern)
        {
          assert(parsePattern());
        }
      
      const std::string pattern() { return _pattern; }

      std::string format(const LogMsg& msg){
        std::stringstream ss;
        format(ss,msg); //父类引用/指针接收子类对象
        return ss.str();
      }
      
      std::ostream& format(std::ostream &os, const LogMsg &msg) {
        for (auto &it : _items) {
          it->format(os, msg);
        }
        return os;
      }

    private:

      std::shared_ptr<FormatItem> createItem(const std::string& key,const std::string& value){
        if(key=="m") return std::make_shared<MsgFormatItem>();
        if(key=="p") return std::make_shared<LevelFormatItem>();
        if(key=="d") return std::make_shared<TimeFormatItem>(value);
        if(key=="c") return std::make_shared<LoggerFormatItem>();
        if(key=="f") return std::make_shared<FileFormatItem>();
        if(key=="l") return std::make_shared<LineFormatItem>();
        if(key=="n") return std::make_shared<NLineFormatItem>();
        if(key=="T") return std::make_shared<TabFormatItem>();
        if(key=="t") return std::make_shared<TidFormatItem>();
        if(key=="") return std::make_shared<OtherFormatItem>(value);
        std::cout<<"规则错误,不是已定义的格式化规则: %"<<key<<"\n";
        abort();
        return nullptr;
      }

      //解析格式化规则
      bool parsePattern(){
        assert(!_pattern.empty());
        //abc%%[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n
        
        // 枚举出所有情况
        // 分析,总结解析规则
        // 寻找解析入口,依次对所有情况进行解析与异常处理
     
        //先解析非格式字符,再解析格式字符,先解析带子项格式字符,再解析不带子项字符

        //开始
        //为空
        //为非格式字符-- 连续处理,一连串为1项
        //为格式字符-- 即跳过非格式字符处理步骤,注意维护
       
        //中间
        //为带子项格式字符
        //不带子项格式字符
        //未定义格式字符
       
        //结果存储
        //处理完一连串非格式字符 -- 存储
        //处理一项格式字符 -- 存储

        //键:格式字符 值:原始字符/格式字符子项
        //解析结果 -- 键值对序列存储 -- 输出

        size_t pos = 0;  

        std::string key; 
        std::string value;
        std::vector<std::pair<std::string,std::string>> order; //键值对序列
        
        while(pos<_pattern.size()){

          //是否% --原始字符
          if(_pattern[pos]!='%'){ //不是%,就是原始字符
            value+=_pattern[pos];
            pos++;
            continue;
          }
          
          //走到这里_pattern[pos]一定是%

          // %%
          if(pos+1<_pattern.size()&&_pattern[pos+1]=='%'){ //%% == 原始%
            value+='%';
            pos+=2;
            continue;
          }

          //走到这里,原始字符串处理完毕
          if(!value.empty()){ //开始为格式字符,即跳过了非格式字符处理流程时,安全处理
            order.push_back(std::make_pair("",value));
            value.clear(); //已存储,清空重新使用
          }

          //走到这里,解析标准格式 
          if(pos+1 == _pattern.size()){
            std::cout<<"规则错误,%之后没有对应的格式字符"<<"\n";
            return false;
          }
          pos+=1;//当前位置是%,往后走一步走到格式字符
          key = _pattern[pos];//存储格式字符key
          pos+=1;//格式字符处理完毕pos++,走到格式字符后一项

          //检查是否是子项,处理带子项的格式字符,如%d{}
          if(pos<_pattern.size()&&_pattern[pos]=='{'){
            pos+=1; //不存储括号
            while(pos<_pattern.size()&&_pattern[pos]!='}'){
              value+=_pattern[pos]; //存储格式字符value
              pos+=1;
            }
            //循环结束,如果此时pos!=_pattern.size(),则处理完毕,如果不是,则说明异常
            //
            //因为不存储花括号,只要在末尾前遇到'}'就会结束循环,末尾时还没匹配到,则说明异常
            if(pos==_pattern.size()){ 
              std::cout<<"规则错误,子规则{}匹配出错"<<"\n";
              return false;
            }
            pos+=1; //此时pos在'}'位置,向后走一步,走到下次处理的位置
          }
          //存储格式字符pair
          order.push_back(std::make_pair(key,value));
          key.clear();
          value.clear();
        }

        //将存储的结果输出--映射关系
        for(auto& it:order){
          //std::cout<<it.first<<" "<<it.second<<"\n";
          _items.push_back(createItem(it.first,it.second));
        }

        return true;
      }


    private:
      std::string _pattern;                            //格式化规则
      std::vector<std::shared_ptr<FormatItem>> _items; //解析好的格式化规则元素序列
  };

} //namespace_log_END

// #endif
