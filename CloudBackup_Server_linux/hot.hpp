#pragma once

#include"data.hpp"
#include"unistd.h"

extern ns_cloud_backup::DataManager* g_dm;

namespace ns_cloud_backup
{
  class HotManaget
  {
    public:
      HotManaget()
      {
        //初始化
        Config* config = Config::GetInstance();
        _backup_dir = config->GetBackupDir();
        _arc_dir = config->GetArcDir();
        _arc_suffix = config->GetArcSuffix();
        _hot_time = config->GetHotTime();
        
        //初次创建目录
        FileUtil fu1(_backup_dir);
        FileUtil fu2(_arc_dir);
        fu1.CreateDirectory();
        fu2.CreateDirectory();
      }
    private:
      bool HotJudge(const std::string &filename)
      {
        FileUtil tmp(filename);
        time_t last_atime = tmp.LastATime(); 
        time_t cur_time = time(nullptr);
        return (cur_time - last_atime > _hot_time) ? false: true;
        //不是热点文件返回false;
        //    热点文件返回true;


      }

    public:
      //启动模块
      bool RunModule()
      {
        while(1)
        {
          //1.遍历获取备份目录下所有文件
          FileUtil fu(_backup_dir);
          std::vector<std::string> bfs;//backup files
          fu.ScanDirectory(bfs);
          for(auto &bf:bfs)
          {
            //2.判断是否热点文件
            if(HotJudge(bf) == true)
            {
              continue; //是热点文件
            }

            //3.获取文件信息
            BackupInfo bi;
            int n = g_dm->GetOneByRealPath(bf,&bi);
            if( n == false )
            {
              bi.NewBackupInfo(bf);//文件信息不存在,则新建一个 --- 新文件
            }

//压缩工作可以交给多线程去做
            //4.压缩文件/删除原文件
            FileUtil tmp(bf);
            tmp.Compress(bi.arc_path); //如果是同名文件,内容会覆盖
            tmp.Remove();

            //5.更新信息 
            bi.arc_flag = true; 
            g_dm->Update(bi);
          }
          usleep(1000);//不要循环太快,占用资源
        }

      }

    private:
      std::string _backup_dir;
      std::string _arc_dir;
      std::string _arc_suffix;
      int _hot_time;
  };



}
