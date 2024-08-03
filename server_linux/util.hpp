#pragma once 

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<experimental/filesystem> //gcc7
//#include<filesystem>            //gcc8
#include<ctime>
#include<cstdio>


#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>

#include"./include/bundle.h"
#include"jsoncpp/json/json.h"
#include<memory>
#include<sstream>

//stat
/*
  常用的 ACM时间+大小
           time_t    st_atime;   / time of last access
           time_t    st_mtime;   / time of last modification
           time_t    st_ctime;   / time of last status change
           off_t     st_size;    / total size, in bytes (整型族)
 */

namespace ns_cloud_backup
{
  inline namespace v1
  {
    namespace fs = std::experimental::filesystem;
    class FileUtil
    {
      private:
        std::string _filename;//文件名称
      public:
        FileUtil(const std::string& filename)
          :_filename(filename)
        {}

        //返回文件大小,类型int64,能防止文件过大,并且能使用-1等作为错误码
        int64_t FileSize()
        {
          struct stat st;
          if(stat(_filename.c_str(),&st)<0)
          {
            std::cout<<"get file size error" <<std::endl;;
            return -1;
          }
          return st.st_size;
        } 

        //文件最后修改时间
        time_t LastMTime()
        {
          struct stat st;
          if(stat(_filename.c_str(),&st)<0)
          {
            std::cout<<"get file last modfly time error" <<std::endl;;
            return -1;
          }
          return st.st_mtime;
        }

        //文件最后访问时间
        time_t LastATime() 
        {
          struct stat st;
          if(stat(_filename.c_str(),&st)<0)
          {
            std::cout<<"get file last access time error" <<std::endl;;
            return -1;
          }
          return st.st_atime;
        }

        //返回不带路径的文件名
        const std::string FileName() 
        {
          //1. 路径格式为./dir/file.txt  .从最后一个'/'开始就是文件名
          //size_t pos = _filename.rfind('/');
          size_t pos = _filename.find_last_of("/"); //最后一个'/'
          if(pos == std::string::npos) //没找到说明直接就是文件名(当前目录)
          {
            return _filename;
          }
          return _filename.substr(pos+1);

        }

        //获取从pos开始,len长度的文件内容
        bool GetPosLen(std::string *body,size_t pos,size_t len)
        {
          //1.打开文件
          std::ifstream ifs;
          ifs.open(_filename,std::ios::binary); //以二进制方式打开,稳定
          //不建议以文本方式,有些字符不一定占一个字节
          if(ifs.is_open() == false)
          {
            std::cout<<"read open file failed!"<<std::endl;
            ifs.close();
            return false;
          }
          //2.判断参数合法性
          size_t fsize = FileSize();
          if(pos+len>fsize)
          {
            std::cout<<"length out of file size"<<std::endl;
            return false;
          }
          //3.调整缓冲区大小,文件指针位置,读取数据
          ifs.seekg(pos,std::ios::beg);
          body->resize(len);
          ifs.read(&(*body)[0],len);
          if(ifs.good() ==  false) //检查流的状态
          {
            std::cout<<"get file content failed!\n";
            ifs.close();
            return false;
          }
          ifs.close();
          return true; 
        }

        //获取文件总体数据,放入body内
        bool GetContent(std::string *body) 
        {

          //1.获取文件总体大小
          size_t fsize = FileSize();
          //2.读取文件,并返回
          return GetPosLen(body,0,fsize);
        }


        //将body写入文件 -- 覆盖
        bool SetContent(const std::string &body)
        {
          //1.打开文件
          std::ofstream ofs;//(_filename,std::ios::binary);
          ofs.open(_filename,std::ios::binary);
          if(ofs.is_open() == false)
          {
            std::cout<<"write open file failed!"<<std::endl;
            ofs.close();
            return false;
          }
          //2.写入文件
          ofs.write(&body[0],body.size());
          if(ofs.good() == false)
          {
            std::cout<<"write file failed!"<<std::endl;
            ofs.close();
            return false;
          }
          ofs.close();
          return true;
        }

        //删除自己
        bool Remove()
        {
          if(Exists() == false)
          {
            return true;
          }
          //C API 删除一个文件
          remove(_filename.c_str());
          return true;
        }


        //返回文件是否存在. ---实例是文件,返回自己是否存在
        bool Exists()
        {
          return fs::exists(_filename);
        }


        //创建文件,连带目录
        bool CreateDirectory()
        {
          if(this->Exists() == true) return true;//判断文件是否存在,不存在则创建之
          return fs::create_directories(_filename);
        }

        //扫面目录中的文件. 实例是目录
        bool ScanDirectory(std::vector<std::string> &array) //引用一个文件数组,存储已存在文件的文件名
        {
          for(auto& p: fs::directory_iterator(_filename)) //语义:遍历filename所在目录的所有文件(包括目录)
          {
            if(!fs::is_directory(p))
              array.push_back(p.path().relative_path().string()); //返回相对路径的字符串
            //array.push_back(fs::path(p).relative_path().string());
          }
          return true;
        }

        //压缩
        bool Compress(const std::string &packname)
        {
          /*
             fileUtil管理的主要是一个要压缩/解压的文件. 接收的参数是压缩包包名
             Compress的工作是,将实例的内容提取出来,压缩,然后放到新文件
             */
          //1.将文件内容提取至缓冲区
          std::string body ;
          if(GetContent(&body) == false)
          {
            std::cout<<"compress read file failed!"<<std::endl;
            return false;
          }
          //2.压缩
          std::string packed = bundle::pack(bundle::LZIP,body);

          //3.将压缩后的数据写入新的文件
          FileUtil fu(packname);
          if(fu.SetContent(packed) == false)
          {
            std::cout<<"compress write packed data failed!\n";
            return false;
          }
          return true;
        }

        bool UnCompress(const std::string &packname)
        {
          /*
             util的实例是一个要解压的文件. 接收的参数是解压包的包名
             UnCompress的工作是,将实例的内容提取出来,解压,然后放到新文件
             */

          //1.将文件内容提取至缓冲区
          std::string body ;
          if(GetContent(&body) == false)
          {
            std::cout<<"uncompress read file failed!"<<std::endl;
            return false;
          }
          //2.解压
          std::string unpacked = bundle::unpack(body);

          //3.将解压后的数据写入新的文件
          FileUtil fu(packname);
          if(fu.SetContent(unpacked) == false)
          {
            std::cout<<"uncompress write packed data failed!\n";
            return false;
          }
          return true;
        }  // Uncompress __End;


    };//class FileUtil ___End;

    class JsonUtil //工具类,没有成员,将几种相近功能的函数封装在一起,类方法为静态(可无需实例化直接调用)
    {
      public:
        static bool Serialize(const Json::Value &root,std::string *str) //输出str
        {
          Json::StreamWriterBuilder swb;
          std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

          std::stringstream ss;
          int ret = sw->write(root,&ss);
          if(ret <0)
          {
            std::cout<<"Serialize write error!"<<std::endl;
            return false;
          }

          *str = ss.str();
          return true;
        }
        static bool UnSerialize(const std::string&str,Json::Value *root)
        {
          Json::CharReaderBuilder crb; //字节读入 建造者
          std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
          std::string err; //error message
          //解析--反序列化
          //parse(const char* beg,const char* end, Json::Value* root,std::string*)
          int ret = cr->parse(str.c_str(),str.c_str()+str.size(),root,&err);
          if(ret <0)
          {
            std::cout<<"UnSerialize parse error : "<<err<<std::endl;
            return false;
          }
          return true; 
        }
    }; //class JsonUtil__Endl;


  } //namespace v1 ___End;
}//namespace ns_cloud_backup __End;
