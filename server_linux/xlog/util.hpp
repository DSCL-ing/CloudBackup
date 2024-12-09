// #ifndef __UTIL_HPP__
// #define __UTIL_HPP__

#pragma once

#include<iostream>
#include<fstream>
#include<chrono>

#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>


namespace xlog{
  namespace util{

    class DateUtil{
      public:
        static time_t getCurTime(){
          return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }
    }; //CLASS_DataUtil_END

    class FileUtil{
      public:
        //判断路径是否存在
        static bool exists(const std::string& pathname){
          //return access(path.c_str(),F_OK) == 0 ? true:false; //unix system call 
          struct stat st;
          return stat(pathname.c_str(),&st) <0 ? false:true;  //unix system call 
        }

        //获取文件所在路径前缀
        static std::string getPath(const std::string& pathname){
          size_t pos = pathname.find_last_of("/\\");
          return pos == std::string::npos? ".":pathname.substr(0,pos+1);
        }

        static void createDirectory(const std::string& pathname){
          //./abc/de/fgh  
          //./
          //.
          //.. 
          //./a
          //abc/efg/h
          //abc/
          //a  -- 直接传目录名或者传空字符串
          size_t pos = 0; //搜索下标
          size_t idx = 0; //下次搜索起始
          while(idx<pathname.size()){
            pos =  pathname.find_first_of("/\\",idx);
            if(std::string::npos  == pos){
              mkdir(pathname.c_str(),0777); //直接传文件名或空字符串
              break; //防止npos+1归零导致死循环
            }
            std::string parent_dir = pathname.substr(0,pos);  //pos-0 == 父目录的长度
            if(!exists(parent_dir)){  // ".", "..", "../", "./"等,都是已存在
              mkdir(parent_dir.c_str(),0777);
            }
            idx = pos +1; 
          }

        }

    }; //CLASS_FileUtil__END

  } //namespace_util_END
} //namespace_Log__END


// #endif
