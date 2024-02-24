#ifndef __MY_CLOUD__
#define __MY_CLOUD__

#include"data.hpp"
#include"httplib.h" //历史遗留问题,httplib内的winsock2.h会和windows.h冲突,需要调换一下位置
#include<windows.h> 
namespace ns_cloud_backup
{
    inline namespace v1
    {
#define SERVER_ADDR "8.134.236.217" //可以写死,实际使用服务器基本不会改变
#define SERVER_PORT 8888
        class Backup
        {
        public:
            Backup(const std::string& backup_dir, const std::string& filename/*实例化DataManager需要*/)
                :_backup_dir(backup_dir)
            {
                _dm = new DataManager(filename);
            }
            bool RunModule()
            {
                while (true)
                {
                    //1.扫描获取指定目录中的所有文件
                    FileUtil fu(_backup_dir);
                    std::vector<std::string> arry;
                    fu.ScanDirectory(arry);
                    //2.逐个判断文件是否需要上传
                    for (auto& a : arry)
                    {
                        if (IsNeedUpload(a) == false)
                        {
                            continue;
                        }
                        else if (Upload(a) == true)
                        {
                            //3.上传文件,并更新数据
                            _dm->Insert(a, GetFileIdentifier(a));
                            std::cout << a << " upload success!" << std::endl;
                        }
                        //上传失败无负担,下次循环会重新上传
                    }
                    Sleep(1);
                }
            }
        private:
            std::string GetFileIdentifier(const std::string& filename)
            {
                //a.txt-fsize-mtime
                FileUtil fu(filename);
                std::stringstream ss;
                ss << fu.FileName() << "-" << fu.FileSize() << "-" << fu.LastMTime();
                return ss.str();
            }
            bool Upload(const std::string& filename)
            {
                //1.获取文件数据 
                FileUtil fu(filename);
                std::string body;
                fu.GetContent(&body);
                //2.构建http客户端对象,填充文件信息
                httplib::Client client(SERVER_ADDR, SERVER_PORT);
                httplib::MultipartFormData item;
                item.name = "file";
                item.filename = fu.FileName();
                item.content = body;
                item.content_type = "application/octet-stream";
                httplib::MultipartFormDataItems items;
                items.push_back(item);
                auto res = client.Post("/upload", items);
                return (!res) ? false : true;
                //if (!res || res->status != 200)
                //{
                //    return false;
                //}
                //return true;
            }

            bool IsNeedUpload(const std::string& filename)
            {
                //判断是否需要上传的条件:
                //1.有无上传记录
                //2.若有,原标记和新标记是否相等
                std::string id;
                if (_dm->GetOneByKey(filename, &id) == true)
                //if (_dm->GetOneByKey(filename, &id) != false)
                {
                    std::string new_id = GetFileIdentifier(filename);
                    if (new_id == id)
                    {
                        return false;
                    }

                }
                //如果一个文件比较大,正在徐徐地拷贝到这个目录下,即拷贝需要一个过程.
                //如果每次遍历都需要上传这个文件,则传一个大文件,几十个G的文件回上传上百次
                //因此,需要一些限制手段
                //1.较为折中:一段时间内没有被修改过的文件才能上传
                //2.判断文件是否被某个进程正在占用
                FileUtil fu(filename);
                if (time(nullptr) - fu.LastMTime() < 3)// 使用方案1: 3秒钟之内没有被修改过才允许上传
                {
                    return false;
                }

                return true;
            }

        private:
            std::string _backup_dir;//备份目录
            DataManager* _dm;       //数据管理

        };
    }
}


#endif