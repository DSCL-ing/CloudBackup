#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__


#include"include/httplib.h"
#include"config.hpp"
#include"data.hpp"

extern ns_cloud_backup::DataManager* g_dm;

namespace ns_cloud_backup
{
  class Service
  {
    public:
      Service()
      {
        Config* config = Config::GetInstance();
        _server_port = config->GetServerPort();
        _server_ip = config->GetServerIP();
        _download_prefix = config->GetUrlPrefix();

      }
      
      bool RunModule()
      {
        _server.Post("/upload",Upload);
        _server.Get("/listshow",listShow);
        _server.Get("/",listShow);
        std::string Download_url = _download_prefix+"(.*)";
        _server.Get(Download_url,Download);
        _server.Get("/test",[](const httplib::Request &req,httplib::Response& rsp){
            rsp.set_content("hello","text/plain");
            rsp.status = 200;
            });
        std::cout<<"service start" <<std::endl;
        _server.listen("0.0.0.0",8888);
        //_server.listen(_server_ip.c_str(),_server_port);
        return true;
      }

    private:
      static void Upload(const httplib::Request &req,httplib::Response& rsp)
      {
        //1.判断文件是否上传成功
        auto ret = req.has_file("file");
        if(ret == false)
        {
          rsp.status = 400;
          return;
        }

        //2.取文件数据
        //注意:multipart上传的文件的正文不全是文件数据,不能直接全部拷贝
        auto file = req.get_file_value("file"); 


        //上传了个空的,不玩了
        if(file.filename == "")
        {
          return ;
        }
        
        //3.把文件数据放到back_dir中
        //需要先取得文件存放路径
        std::string backup_dir = Config::GetInstance()->GetBackupDir(); 
        std::string realpath = backup_dir+FileUtil(file.filename).FileName();
        FileUtil fu(realpath);
        fu.SetContent(file.content);
        
        //4.更新文件信息
        BackupInfo bi;
        bi.NewBackupInfo(realpath);
        g_dm->Insert(bi);
      }

      static std::string GetETag(const BackupInfo &info)
      {
        // ETag:filename-fsize-mtime
        FileUtil fu(info.real_path);
        std::string etag = fu.FileName();
        etag+="-";
        etag+=fu.FileSize();
        etag+="-";
        etag+=fu.LastMTime();
        return etag;
      }

      static void Download(const httplib::Request &req,httplib::Response& rsp)
      {
        //1.获取客户端请求的资源路径path -- req成员
        //2.根据资源路径,获取文件备份信息
        BackupInfo info; 
        g_dm->GetOneByURL(req.path,&info);
        //3.判断文件是否被压缩,如果被压缩,要先解压缩--> 非热点文件
        //    如果压缩,则还需要修改备份信息,然后删除压缩包
        if(info.arc_flag == true)
        {
          FileUtil fu(info.arc_path);
          fu.UnCompress(info.real_path); //解压到backup_path中
          info.arc_flag = false;
          g_dm->Update(info);
          fu.Remove();
        }
        //4.读取文件数据,放入rsp.body中
        FileUtil fu(info.real_path);
        fu.GetContent(&rsp.body);


        //5.断点续传
        //Req中只有If-Range字段且etag一致,才是断点续传.否则是正常全文下载
        std::string old_etag;
        bool retrans = false;
        if(req.has_header("If-Range"))
        {
          old_etag = req.get_header_value("If-Range");
          if(old_etag == GetETag(info))
          {
            retrans = true;
          }
        }
        
        //6.设置响应头部字段:ETag,Accept-Ranges:bytes
        if(retrans == false)
        {
          //正常下载
          rsp.set_header("ETag",GetETag(info));
          rsp.set_header("Accept-Ranges","bytes");//允许断点续传
          rsp.set_header("Content-Type","application/octet-stream");
          rsp.status = 200;
        }
        else
        {
          //断点续传 --- httplib内部实现了区间请求,即断点续传请求的处理
          //只需要用户将所有数据读取到rsp.body中,就会自动取出指定区间数据进行响应
          //本质实现: std::string range = req.get_header-val("Range");//内容是bytes=start-end(起点到终点).然后解析range字符串就能得到请求区间了....
          rsp.set_header("ETag",GetETag(info));
          rsp.set_header("Accept-Ranges","bytes");//允许断点续传
          rsp.set_header("Content-Type","application/octet-stream");
          rsp.status = 206;
          //rep.set_header("Content-Range","bytes start-end/fsize");//手动解析需要填这个字段
        }
      }

      static std::string TimeToStr(time_t t)
      {
        return ctime(&t);
      }

      static void listShow(const httplib::Request &req,httplib::Response& rsp)
      {
        //1.获取所有文件信息
        std::vector<BackupInfo> infos;
        g_dm->GetAll(&infos);

        //2.构建html
        std::stringstream ss;
        ss<<"<html><head><title>Download</title></head>";
        ss<<"<body><h1>Download</h1><table>";
        ss<<"<tr>";
        //遍历信息表,每一行放一个文件信息
        for(auto &info :infos)
        {
          //html不区分单双引号
          std::string filename = FileUtil(info.real_path).FileName();
          ss<<"<td><a href='"<<info.url<<"'>"<<filename<<"</a></td>";
          ss<<"<td align='right'> "<<TimeToStr(info.atime)<<" </td>";
          ss<<"<td align='right'> "<<info.fsize/1024<<"k</td>";

        ss<<"</tr>";
        }
        ss<<"</table></body></html>";
        rsp.set_content(ss.str(),"text/html");
        rsp.status = 200;
        return ;
      }

    private:
      int _server_port;
      std::string _server_ip;
      std::string _download_prefix;
      httplib::Server _server;
  };

}
#endif
