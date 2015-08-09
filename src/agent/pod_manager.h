#ifndef POD_MANAGER_H
#define POD_MANAGER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include "mutex.h"
#include "pod_info.h"

namespace baidu {

namespace common {
class Thread;
}

namespace galaxy {

class TaskManager;
class InitdHandler;

class PodManager {
public:
    PodManager();

    ~PodManager();

    int Run(const PodDesc& pod);

    int Kill(const PodDesc& pod);

    int Query(const std::string& podid, 
              boost::shared_ptr<PodInfo>& info);

    int List(std::vector<std::string>* pod_ids);

private:
    enum Operation {
        kCreate,
        kDelete,
    };

    typedef std::map<std::string, boost::shared_ptr<PodInfo> > PodInfosType; 
    typedef std::map<std::string, boost::shared_ptr<InitdHandler> > InitdHandlersType; 
    
private:
    int Load();

    int Dump();

    int Set(PodInfo* info);

    int Delete(const std::string& podid);


    int FesibilityCheck(const Resource& resource);

    int DoPodOperation(const PodDesc& pod, const Operation op);

    int InstallPackage();

    void LoopCheckPodInfos();

private:
    Mutex infos_mutex_;
    PodInfosType pod_infos_;

    Mutex handlers_mutex_;
    InitdHandlersType initd_handlers_;

    boost::scoped_ptr<TaskManager> task_manager_;

    boost::scoped_ptr<common::Thread> initd_check_thread_;
};

}
}

#endif
