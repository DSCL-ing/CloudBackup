#include"util.hpp"
#include"config.hpp"
#include"data.hpp"
#include"hot.hpp"
#include<memory>
#include"service.hpp"
#include<thread>

namespace cloud = ns_cloud_backup;

void FileUtilTest(std::string filename)
{
  //ns_cloud_backup::FileUtil fu(filename);
  //std::cout<<fu.FileName()<<std::endl;
  //std::cout<<fu.FileSize()<<std::endl;
  //std::cout<<fu.LastATime()<<std::endl;
  //std::cout<<fu.LastMTime()<<std::endl;

  //cloud::FileUtil fu(filename);
  //fu.Compress("1");
  //fu.UnCompress("2");

  //ns_cloud_backup::FileUtil fu(filename);
  //fu.CreateDirectory();
  //
  //std::vector<std::string> v;
  //fu.ScanDirectory(v);
  //for(std::string &s:v)
  //{
  //  std::cout<<s<<std::endl;
  //}
  
}


void JsonUtilTest()
{
  const char* name = "xiaoming";
  int age = 18;
  float score[] = {85,88.5,99};
  Json::Value root;
  root["name"] = name;
  root["age"] = 18;
  root["score"].append(score[0]);
  root["score"].append(score[1]);
  root["score"].append(score[2]);
  std::string json_str;
  cloud::JsonUtil::Serialize(root,&json_str);
  std::cout<<json_str<<std::endl;  

  Json::Value res;
  cloud::JsonUtil::UnSerialize(json_str,&res);
  std::cout<<res.toStyledString()<<std::endl; 
}

void ConfigTest()
{
  cloud::Config* con = cloud::Config::GetInstance();
  std::cout<<con->GetHotTime()<<std::endl;
  std::cout<<con->GetServerPort()<<std::endl;
  std::cout<<con->GetServerIP()<<std::endl;
  std::cout<<con->GetUrlPrefix()<<std::endl;
  std::cout<<con->GetArcSuffix()<<std::endl;
  std::cout<<con->GetBackupDir()<<std::endl;
  std::cout<<con->GetArcDir()<<std::endl;
  std::cout<<con->GetBackupFileName()<<std::endl;
}

void DataInfoTest()
{
  cloud::BackupInfo bi;
  bi.NewBackupInfo("./cloud.cpp");
  std::cout<<
    bi.arc_flag<<std::endl<<
    bi.fsize<<std::endl<<
    bi.mtime<<std::endl<<
    bi.atime<<std::endl<<
    bi.real_path<<std::endl<<
    bi.arc_path<<std::endl<<
    bi.url<< std::endl;
}
void PrintBackupInfo(cloud::BackupInfo&bi)
{
  std::cout<<
    bi.arc_flag<<std::endl<<
    bi.fsize<<std::endl<<
    bi.mtime<<std::endl<<
    bi.atime<<std::endl<<
    bi.real_path<<std::endl<<
    bi.arc_path<<std::endl<<
    bi.url<< std::endl;
}

void DataManagerTest()
{
  cloud::DataManager dm;
  cloud::BackupInfo bi; 
  bi.NewBackupInfo("./cloud.cpp"); //目前感觉写成构造函数更好
  bi.arc_flag = true;
  dm.Insert(bi);
  
  bi.NewBackupInfo("./config.hpp");
  bi.arc_flag = true;
  dm.Update(bi);
  
  
  std::cout<<"-----------------get Info by url -------------"<<std::endl;
  cloud::BackupInfo tmp;
  dm.GetOneByURL("/download/config.hpp",&tmp);
  PrintBackupInfo(tmp);

  std::cout<<"-----------------get Info by real path -------------"<<std::endl;
  dm.GetOneByRealPath("./cloud.cpp",&tmp);
  PrintBackupInfo(tmp);
  
  std::cout<<"-----------------get all Info  -------------"<<std::endl;
  std::vector<cloud::BackupInfo> v;
  dm.GetAll(&v);
  for(auto &info :v)
  {
    PrintBackupInfo(info);
  }
}
void DataManagerTest2() //test func:InitLoad()
{
  std::cout<<"-----------------get all Info  -------------"<<std::endl;
  std::vector<cloud::BackupInfo> v;
  cloud::DataManager dm;
  dm.GetAll(&v);
  for(auto &info :v)
  {
    PrintBackupInfo(info);
  }
  
}

ns_cloud_backup::DataManager* g_dm;

//全局类没必要上智能指针
//std::unique_ptr<ns_cloud_backup::DataManager> g_dm(new ns_cloud_backup::DataManager);

void HotTest()
{
  cloud::HotManaget hm;
  hm.RunModule();
}

void ServiceTest()
{
  cloud::Service service; 
  service.RunModule();
}

int main(int argc, char* argv[])
{
  g_dm = new ns_cloud_backup::DataManager;
  //FileUtilTest(argv[1]);
  //JsonUtilTest();
  //ConfigTest();
  //DataInfoTest();
  //DataManagerTest();
  //DataManagerTest2();
  //HotTest();
  //ServiceTest();
  
  std::thread thread_hot_manager(HotTest);
  std::thread thread_service(ServiceTest);

  thread_hot_manager.join();
  thread_service.join();

  return 0;
}
