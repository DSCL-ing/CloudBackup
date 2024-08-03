#ifndef __DATA_HPP__
#define __DATA_HPP__

#include<pthread.h>
#include<unordered_map>
#include<vector>
#include<mutex>
#include"util.hpp"
#include"config.hpp"

namespace ns_cloud_backup{

  //存储备份文件信息的结构
  struct BackupInfo
  { 
    bool _arc_flag; //是否被压缩
    size_t _fsize;  //原文件大小
    time_t _mtime;  //last modify修改时间
    time_t _atime;  //last access访问时间
    std::string _real_path; //实际路径
    std::string _arc_path;  //压缩包路径: ./压缩包路径/文件.压缩包后缀名
    std::string _url;  //下载资源路径

    //构建一个备份文件信息结构
    bool NewBackupInfo(const std::string &realpath) // ./dir/filename
    {
      FileUtil fu(realpath);
      if(fu.Exists() == false) 
      {
        std::cout<<"NewBackupInfo : file not exists"<<std::endl;
        return false;
      }
      _fsize = fu.FileSize();
      _mtime = fu.LastMTime(); 
      _atime = fu.LastATime();

      _arc_flag = false;
      _real_path = realpath;

      Config*config = Config::GetInstance();
      std::string arc_dir = config->GetArcDir();
      std::string arc_suffix = config->GetArcSuffix();
      // ./压缩包路径/a.txt.压缩包后缀名
      _arc_path =arc_dir+fu.FileName()+arc_suffix;
      // /download/a.txt
      _url = config->GetUrlPrefix()+ fu.FileName();

      return true;
    }

  };

  class DataManager
  {
//读写锁与排他锁(独占锁)不同的是，读写锁在同一时刻可以允许多个读线程方法，但是在写线程访问时，所有的读线程和其它写线程均被阻塞。读写锁维护了一对锁，一个读锁和一个写锁，通过分离读锁和写锁，并发性相比一般的排它锁有了很大的提升。
//一般情况下，读写锁的性能都会比排它锁好，因为在多数场景读是多于写的。在读多写少的场景中，读写锁提供比排它锁更好的并发性和吞吐量。
    private:
      std::string _backup_file;//存储备份信息的文件 "cloud.dat"
      pthread_rwlock_t _rwlock;//读写锁,同时读,互斥写

      //使用url作为key:客户端浏览器下载文件的时候总是以url作为请求。
      std::unordered_map<std::string,BackupInfo> _table; //key是url

    public:
      DataManager()
      {
        _backup_file = Config::GetInstance()->GetBackupFileName();//cloud.dat
        pthread_rwlock_init(&_rwlock,nullptr);
        //pthread_mutex_init(&_mtx,nullptr);
        InitLoad();
      } 
      ~DataManager()
      {
        pthread_rwlock_destroy(&_rwlock);
        //pthread_mutex_destroy(&_mtx);
      }
      
      //将备份信息读取到内存
      bool InitLoad()
      {

        //加载到内存
        FileUtil fu(_backup_file);
        if(fu.Exists() == false)
        {
          std::cout<<R"comment(InitLoad: file "cloud.dat" not found)comment"<<std::endl;
          return false;
        }
        std::string body; 
        fu.GetContent(&body); //自动扩容

        //反序列化
        Json::Value root;
        JsonUtil::UnSerialize(body,&root); //反序列化到json
        for(int i = 0;i<(int)root.size();i++)
        {
          BackupInfo info; 
          info._arc_flag = root[i]["arc_flag"].asBool();
          info._fsize = root[i]["fsize"].asInt64();
          info._atime = root[i]["atime"].asInt64();
          info._mtime = root[i]["mtime"].asInt64();
          info._real_path = root[i]["real_path"].asString();
          info._arc_path = root[i]["arc_path"].asString();
          info._url = root[i]["url"].asString();
          Insert(info);
        }
        return true;

      }
     
      //新增文件信息节点
      bool Insert(const BackupInfo&info)
      {
        pthread_rwlock_wrlock(&_rwlock);
        //pthread_mutex_lock(&_mtx);
        _table[info._url] = info;
        pthread_rwlock_unlock(&_rwlock);
        //pthread_mutex_unlock(&_mtx);
        Storage();
        return true;
      }
      
      //更新文件信息节点
      bool Update(const BackupInfo&info) //目前和insert没有区别
      {
        //pthread_mutex_lock(&_mtx);
        pthread_rwlock_wrlock(&_rwlock);
        _table[info._url] = info; //覆盖
        pthread_rwlock_unlock(&_rwlock);
        //pthread_mutex_unlock(&_mtx);
        Storage();
        return true;
      }
      
      //根据文件的URL获取信息节点
      bool GetOneByURL(const std::string& url,BackupInfo*info)//根据url获取一条info
      {
        //pthread_mutex_lock(&_mtx);
        pthread_rwlock_rdlock(&_rwlock);
        auto it = _table.find(url);
        if(it == _table.end()) 
        {
          //pthread_mutex_unlock(&_mtx);
          pthread_rwlock_unlock(&_rwlock);
          return false;
        }
        *info = it->second;
        //pthread_mutex_unlock(&_mtx);
        pthread_rwlock_unlock(&_rwlock);
        return true;
      }
      //根据文件的实际路径获取信息节点
      bool GetOneByRealPath(const std::string realpath,BackupInfo*info)//根据realpath获取一条info
      {
        //pthread_mutex_lock(&_mtx);
        pthread_rwlock_rdlock(&_rwlock);
        //pthread_rwlock_rdlock(&_rwlock);
        for(auto it:_table)
        {
          if(it.second._real_path == realpath)
          {
            *info = it.second;
            //pthread_mutex_unlock(&_mtx);
            pthread_rwlock_unlock(&_rwlock);
            return true; //存在返回真
          }
        }
        //pthread_mutex_unlock(&_mtx);
        pthread_rwlock_unlock(&_rwlock);
        return false;//文件不存在
      }

      //获取所有文件的信息结构
      bool GetAll(std::vector<BackupInfo> *arry)//获取所有的info放入vector
      {
        //pthread_mutex_lock(&_mtx);
        pthread_rwlock_rdlock(&_rwlock);
        if(_table.empty())
        {
          pthread_rwlock_unlock(&_rwlock);
          //pthread_mutex_unlock(&_mtx);
          return false;
        }
        for(auto it:_table)
        {
          arry->push_back(it.second);
        }
        pthread_rwlock_unlock(&_rwlock);
        //pthread_mutex_unlock(&_mtx);
        return true;
      }

      
      bool Storage() //持久化,每有信息改变(Insert,Update)就需要持久化一次
      {
        //1.获取所有配置信息
        //2.遍历添加到Json root中,每个配置对象作为一个Json item数组
        //3.json序列化
        //4.以json格式写入到文件中
        std::vector<BackupInfo> infos;
        GetAll(&infos);
        Json::Value root;
        for(auto info : infos)
        {
          Json::Value item;
          item["fsize"] = (Json::Int64)info._fsize;
          item["atime"] = (Json::Int64)info._atime;
          item["mtime"] = (Json::Int64)info._mtime;
          item["arc_flag"] = info._arc_flag;
          item["real_path"] = info._real_path;
          item["arc_path"] = info._arc_path;
          item["url"] = info._url;
          root.append(item);
        }
        std::string body;
        JsonUtil::Serialize(root,&body);
        FileUtil fu(_backup_file);
        fu.SetContent(body);
        return true;
      }
  };

}

#endif
