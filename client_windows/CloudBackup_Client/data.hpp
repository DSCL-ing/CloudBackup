#ifndef __DATA_HPP__
#define __DATA_HPP__

#include"util.hpp"
#include<unordered_map>
#include<sstream>

namespace ns_cloud_backup
{
    inline namespace v1
    {
        class DataManager
        {
        public:
            DataManager(const std::string backup_file) :_backup_file(backup_file)
            {
                InitLoad();
            }

            bool Storage()//持久化存储
            {
                //1.获取所有备份信息
                //2.将所有信息组织成持久化存储的格式
                //3.持久化存储

               //std::string body;
                std::stringstream ss; //字符串流组织格式更方便 
                for (auto& pair : _table)
                {
                    //body+=pair.first;
                    //body+=" ";
                    //body+=pair.second;
                    //body+="\n";
                    ss << pair.first << " " << pair.second << "\n";
                }
                FileUtil fu(_backup_file);
                fu.SetContent(ss.str());
                return true;
            }

            int Split(const std::string& str, const std::string& sep, std::vector<std::string>* arry)
            {
                //围绕find和substr,将字符串分割成一个个元素
                int count = 0;
                size_t idx = 0, pos = 0;
                //while (idx < str.size())
                while (true) //死循环好控制,减少考虑边界
                {
                    pos = str.find(sep, idx);//find(要查找的子串,查找的起始位置)
                    if (pos == std::string::npos) //找不到了,说明是最后一段,在循环外执行最后的插入 "SEP str"
                    {
                        break;
                    }
                    if (idx == pos) // 防止因为连续的分隔符而插入空白子串 "... str SEP SEP str ..."
                    {
                        idx = pos + sep.size();
                        continue;
                    }

                    std::string substr = str.substr(idx, pos - idx);//substr(起始位置,子串长度);
                    arry->push_back(substr);
                    idx = pos + sep.size();
                    count++;
                }

                if (idx < str.size()) //不能插入的情况:最后一段后面还带分隔符 -- "str SEP"
                {
                    arry->push_back(str.substr(idx));//substr(起始位置);从起始位置开始截取到字符串末尾
                    count++;
                }
                return count;
            }

            bool InitLoad()//启动时加载
            {
                //1.从文件中读取所有数据
                //2.解析数据,存入表中
                FileUtil fu(_backup_file);
                std::string body;
                fu.GetContent(&body);

                //解析数据,并存表
                std::vector<std::string> arry;
                Split(body, "\n", &arry);
                for (auto& str : arry)
                {
                    std::vector<std::string> tmp;
                    Split(str, " ", &tmp);
                    if (tmp.size() != 2) //说明分割有问题
                    {
                        continue;//舍弃
                    }
                    _table[tmp[0]] = tmp[1]; //_table[key] = val;
                }
                return true;
            }

            bool Insert(const std::string& key, const std::string& val)
            {
                //客户端使用的单线程,无线程安全问题,所以没有使用锁
                _table[key] = val;
                Storage();
                return true;
            }

            bool Update(const std::string& key, const std::string& val)
            {
                _table[key] = val;
                Storage();
                return true;
            }

            bool GetOneByKey(const std::string& key, std::string* val)
            {
                //map.find
                auto it = _table.find(key);
                if (it == _table.end())
                {
                    return false;
                }
                *val = it->second;
                return true;
            }

        private:
            std::string _backup_file;//备份信息的持久化存储文件
            std::unordered_map<std::string, std::string> _table;
        };

    }
}





#endif