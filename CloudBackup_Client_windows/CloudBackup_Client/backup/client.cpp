#include"util.hpp"
#include"data.hpp"
#include"cloud.hpp"

#define BACKUP_FILE "./backup.dat"
#define BACKUP_DIR "./backup/"

int main()
{
    //ns_cloud_backup::FileUtil fu("./");
    //std::vector<std::string> arry;
    //fu.ScanDirectory(arry);
    //ns_cloud_backup::DataManager dm(BACKUP_FILE);
    //for (auto& i : arry)
    //{
    //    dm.Insert(i,"fajlkjgldkajlkfjadlk");
    //}


    //ns_cloud_backup::DataManager dm(BACKUP_FILE);
    //std::string val;
    //dm.GetOneByKey(".\\util.hpp",&val);
    //std::cout<<val<<std::endl;

    ns_cloud_backup::Backup backup(BACKUP_DIR,BACKUP_FILE);
    backup.RunModule();

    return 0;
}