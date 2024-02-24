#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__


#include"util.hpp"
#include<mutex>
#include<ctime>

#define CONFIG_FILE_PATH "./config/cloud.conf" //全局可见

namespace ns_cloud_backup
{
  class Config
  {
    public:
      static Config* GetInstance()
      {
        if(_instance == nullptr)
        {
          std::lock_guard<std::mutex> lg(_mtx);
          if(_instance == nullptr)
          {
            _instance = new Config();
          }
        }
        return _instance;
      }

    public: 
      std::string GetServerIP()
      {
        return _server_ip;
      }
      int GetServerPort()
      {
        return _server_port;
      }
      time_t GetHotTime()
      {
        return _hot_time;
      }
      std::string GetUrlPrefix()
      {
        return _url_prefix;
      }
      std::string GetArcSuffix()
      {
        return _arc_suffix;
      }
      std::string GetBackupDir()
      {
        return _back_dir;
      }
      std::string GetArcDir()
      {
        return _arc_dir;
      }
      std::string GetBackupFileName()
      {
        return _backup_file;
      }

    private:
      Config() 
      {
        int ret = ReadConfigfile();
        if(ret == false)
        {
          std::cout<<"Config constuct ReadConfigfile error"<<std::endl;
          exit(0);
        }
      }
      Config(const Config& c) = delete;

      bool ReadConfigfile()
      {
        std::string body;
        FileUtil fu(CONFIG_FILE_PATH);
        if(fu.GetContent(&body) == false)
        {
          std::cout<<"load config file error"<<std::endl;
          return false;
        }

        Json::Value root;
        JsonUtil ju; 
        if(ju.UnSerialize(body,&root) == false)
        {
          std::cout<<"UnSerialize config file error"<<std::endl;
          return false;
        }
        _hot_time = root["hot_time"].asInt();
        _server_port = root["server_port"].asInt();
        _server_ip = root["server_ip"].asString();
        _url_prefix = root["url_prefix"].asString();
        _arc_suffix = root["arc_suffix"].asString();
        _backup_file = root["backup_file"].asString();
        _back_dir = root["back_dir"].asString();
        _arc_dir = root["arc_dir"].asString();
        return true;
      }

    private:
      time_t _hot_time; //热点判断时间
      int _server_port;
      std::string _server_ip;
      std::string _url_prefix; //下载资源前缀
      std::string _arc_suffix; //压缩包后缀
      std::string _arc_dir; //压缩包路径
      std::string _back_dir; //备份文件路径
      std::string _backup_file;//配置文件名



    private:
      static std::mutex _mtx;
      static Config* _instance; 

  };
  std::mutex Config::_mtx;
  Config* Config::_instance = nullptr;
}
#endif
