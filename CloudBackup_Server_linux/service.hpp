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
        //加载配置信息
        Config* config = Config::GetInstance();
        _server_port = config->GetServerPort();
        _server_ip = config->GetServerIP();
        _download_prefix = config->GetUrlPrefix();
      }
      
      //运行Serveic模块
      bool RunModule()
      {
        //注册POST/GET请求处理函数：
        
        _server.Post("/upload",Upload);//上传
        _server.Get("/listshow",listShow);//显示列表
        _server.Get("/",listShow);

        // /download/*
        std::string Download_url = _download_prefix+"(.*)";
        _server.Get(Download_url,Download); //下载
        
        _server.Get("/test",[](const httplib::Request& req,httplib::Response& rsp){
            (void)req;
            rsp.set_content("hello","text/plain");
            rsp.status = 200;
            });

        std::cout<<"service start" <<std::endl;
        _server.listen("0.0.0.0",8888); //云服务器不允许直接绑定ip,任意ip(交给云服务器),端口号为8888
        //_server.listen(_server_ip.c_str(),_server_port);
        return true;
      }

    private:
      static void Upload(const httplib::Request& req,httplib::Response& rsp)
      {
        std::cout<<"Upload..."<<std::endl;
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
        //file = httplib::MultipartFormData:multipart中的载荷部分


        //上传了个空的,就直接返回,什么都不做
        if(file.filename == "")
        {
          return ;
        }
        
        //3.把文件数据放到back_dir中
        //构建路径
        std::string backup_dir = Config::GetInstance()->GetBackupDir(); 
        std::string realpath = backup_dir+FileUtil(file.filename).FileName();
        //写入
        FileUtil fu(realpath);
        fu.SetContent(file.content);
        
        //4.更新文件信息
        BackupInfo bi;
        bi.NewBackupInfo(realpath);
        g_dm->Insert(bi);
        std::cout<<"文件上传:"<<file.filename<<std::endl;
      }

      //ETag:Entity Tag:实体标签
      //验证资源的唯一标识,可以是字符串(弱)和散列值(强)
      static std::string GetETag(const BackupInfo& info)
      {
        // ETag:filename-fsize-mtime(字符串)
        FileUtil fu(info._real_path);
        std::string etag = fu.FileName();
        etag+="-";
        etag+=fu.FileSize();
        etag+="-";
        etag+=fu.LastMTime();
        return etag;
      }

      static void Download(const httplib::Request& req,httplib::Response& rsp)
      {
        //1.获取客户端请求的资源路径path -- req成员
        //2.根据资源路径,获取文件备份信息
        BackupInfo info; 
        g_dm->GetOneByURL(req.path,&info);
        //3.判断文件是否被压缩,如果被压缩,要先解压缩--> 非热点文件
        //    如果是压缩文件,则还需要修改备份信息,然后删除压缩包
        if(info._arc_flag == true)
        {
          FileUtil fu(info._arc_path);
          fu.UnCompress(info._real_path); //解压到backup_path中
          info._arc_flag = false;
          g_dm->Update(info);
          fu.Remove();
        }
        //4.读取文件数据,放入rsp.body中
        FileUtil fu(info._real_path);
        fu.GetContent(&rsp.body);

//有些网站刚点击时,0kb卡一段时间,不一定是真卡了,可能是在解压中

        //5.断点续传
        //Req中有If-Range字段且etag一致,是断点续传的前置条件.否则是正常全文下载
        //If-Range:用于创建具有条件的范围请求
        //If-Range 头的值可以是一个日期时间（对应 If-Modified-Since）或实体标签（ETag，对应 If-None-Match）,二选1
        std::string old_etag;
        bool retrans = false; //retrans:重试,重传
        
        //判断是否有If-Range字段(标头),has_header
        if(req.has_header("If-Range"))
        {
          old_etag = req.get_header_value("If-Range");
          if(old_etag == GetETag(info)) {
            retrans = true;//比较"If-Range"的值(ETag)是否一致,一致则可以范围请求
          }
        }
        
        //6.设置响应头部字段Accept-Ranges如何范围操作:
        //如果不能重传,则走正常下载,不能断点续传
        if(retrans == false)
        {
          //正常下载
          rsp.set_header("ETag",GetETag(info));
          rsp.set_header("Accept-Ranges","bytes");//设置允许范围操作,以字节方式(也是默认方式)
          rsp.set_header("Content-Type","application/octet-stream"); //设置内容类型为二进制流,且不需要解析
          rsp.status = 200;
        }
        else
        {
          //断点续传 --- httplib内部实现了区间请求,即断点续传请求的处理
          //只需要用户将所有数据读取到rsp.body中,就会自动取出指定区间数据进行响应
          //实现: std::string range = req.get_header-val("Range");//内容是bytes=start-end(起点到终点).然后解析range字符串就能得到请求区间了....
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
        (void)req;
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
          std::string filename = FileUtil(info._real_path).FileName();
          ss<<"<td><a href='"<<info._url<<"'>"<<filename<<"</a></td>";
          ss<<"<td align='right'> "<<TimeToStr(info._atime)<<" </td>";
          ss<<"<td align='right'> "<<info._fsize/1024<<"k</td>";

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
