#pragma once

#include"data.hpp"
#include"unistd.h"

extern ns_cloud_backup::DataManager* g_dm;

//非热点文件长时间不使用,压缩节省资源

namespace ns_cloud_backup
{
  class HotManaget
  {
    private:
      std::string _backup_dir;
      std::string _arc_dir;
      std::string _arc_suffix;
      int _hot_time;
    
    public:
      HotManaget()
      {
        //初始化
        Config* config = Config::GetInstance();
        _backup_dir = config->GetBackupDir();
        _arc_dir = config->GetArcDir();
        _arc_suffix = config->GetArcSuffix();
        _hot_time = config->GetHotTime();
        
        //初次启动时创建目录
        FileUtil fu1(_backup_dir);
        FileUtil fu2(_arc_dir);
        fu1.CreateDirectory();
        fu2.CreateDirectory();
      }
    private:
      //判断是否热点文件
      bool HotJudge(const std::string &filename)
      {
        // 超过一定时间不再访问的文件则为非热点文件
        FileUtil tmp(filename);
        time_t last_atime = tmp.LastATime(); 
        time_t cur_time = time(nullptr);
        return (cur_time - last_atime > _hot_time) ? false: true;
        //不是热点文件返回false;
        //    热点文件返回true;


      }

    public:
      //启动热点判断模块
      bool RunModule()
      {
        while(1)
        {
          //1.遍历获取备份目录下所有文件
          FileUtil fu(_backup_dir);
          std::vector<std::string> bfs;//backup files
          fu.ScanDirectory(bfs); //扫描_backup_dir中所有文件,将文件名存入数组

          //开始处理
          for(auto &bf:bfs)
          {
            //2.判断是否热点文件
            if(HotJudge(bf) == true)
            {
              continue; //是热点文件则不需要特别处理
            }

            //3.获取文件信息
            BackupInfo bi;
            int n = g_dm->GetOneByRealPath(bf,&bi);
            if( n == false )
            {
              bi.NewBackupInfo(bf);//文件信息不存在,则新建一个 --- 新文件
            }

//(扩展:压缩工作可以交给多线程去做)
            //4.对热点文件进行压缩处理
            FileUtil tmp(bf);
            tmp.Compress(bi._arc_path); //如果是同名文件,内容会覆盖
            
            //删除原文件
            tmp.Remove();

            //5.更新信息 
            bi._arc_flag = true; //是否已被压缩:是
            g_dm->Update(bi);
          }
          usleep(1000);//不要循环太快,占用资源
        }

      }

  };



}
