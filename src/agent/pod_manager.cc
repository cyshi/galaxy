#include "pod_manager.h"
#include <boost/bind.hpp>
#include "gflags/gflags.h"
#include "proto/galaxy.pb.h"
#include "thread.h"
#include "utils.h"
#include "task_manager.h"
#include "initd_handler.h"

DECLARE_string(gce_work_dir);

namespace baidu {
namespace galaxy {

PodManager::PodManager() {
    task_manager_.reset(new TaskManager());
    initd_check_thread_.reset(new common::Thread());
    initd_check_thread_->Start(
        boost::bind(&PodManager::LoopCheckPodInfos, this));
}

PodManager::~PodManager() {
}

int PodManager::InstallPackage() {
    return 0;
}

int PodManager::Run(const PodDesc& pod) {
    int ret = 0;
    {
    MutexLock lock(&infos_mutex_);
    PodInfosType::iterator it = pod_infos_.find(pod.id);
    if (it != pod_infos_.end()) {
        LOG(INFO, "pod[%s] already exist", pod.id.c_str());
        return ret;
    }    
    }

    ret = file::Mkdir(FLAGS_gce_work_dir.c_str());
    if (ret != 0) {
        LOG(INFO, "work dir already exist[%s]", FLAGS_gce_work_dir.c_str());
    }
    ret = file::Mkdir(FLAGS_gce_work_dir + "/" + pod.id);
    if (ret != 0) {
        LOG(INFO, "pod dir already exist[%s]", pod.id.c_str());
    }

    // create intid handler
    boost::shared_ptr<InitdHandler> handler(new InitdHandler());

    // fork initd process
    ret = handler->Create(pod.id, FLAGS_gce_work_dir);

    if (ret != 0) {
        LOG(WARNING, "create initd handler error[%d]", ret);
        return ret;
    }

    {
    MutexLock lock(&handlers_mutex_);
    initd_handlers_[pod.id] = handler;
    }


    // create pod info
    boost::shared_ptr<PodInfo> pod_info(new PodInfo());
    pod_info->port = handler->GetPort();
    pod_info->desc = pod;
    // TODO
    pod_info->status.set_state(kPodPending);

    {
    MutexLock lock(&infos_mutex_);
    pod_infos_[pod.id] = pod_info;
    }

    LOG(INFO, "create pod[%s] success", pod.id.c_str());

    return ret;
}

int PodManager::Kill(const PodDesc& pod) {
    int ret = DoPodOperation(pod, kDelete);
    return ret;
}

int PodManager::Query(const std::string& podid, 
                      boost::shared_ptr<PodInfo>& info)  {

    {
    MutexLock lock(&infos_mutex_);
    PodInfosType::iterator it = pod_infos_.find(podid);
    if (it == pod_infos_.end()) {
        // not found
        LOG(INFO, "not found pod[%s]", podid.c_str());
        return -1;
    } else {
        *info = *(it->second);
    }
    } // end lock
    return 0;
}

int PodManager::List(std::vector<std::string>* pod_ids) {
    if (pod_ids == NULL) {
        return -1;
    }

    {
    MutexLock lock(&infos_mutex_);
    for (PodInfosType::iterator it = pod_infos_.begin(); 
         it != pod_infos_.end(); ++it) {
        pod_ids->push_back(it->first);
    }
    }
    return 0;
}

int PodManager::DoPodOperation(const PodDesc& pod, 
                               const Operation op) {
    int ret = 0; 
    // boost::shared_ptr<InitdHandler> handler;
    // {
    // MutexLock lock(&handlers_mutex_);
    // PodHandlersType::iterator it = pod_handlers_.find(pod.id);
    // 
    // // not found
    // if (it == pod_handlers_.end()) {
    //     handler.reset(new InitdHandler());
    //     pod_handlers_[pod.id] = handler;
    // } else {
    //     handler = it->second;
    // }
    // }

    // switch (op) {
    // case kCreate:
    //     ret = handler->Create(pod);
    //     break;
    // case kDelete:
    //     ret = handler->Delete();
    //     break;
    // default:
    //     ret = kUnknown;
    //     break;
    // }
    return ret;
}

int PodManager::FesibilityCheck(const Resource& resource) {
    return 0; 
}

void PodManager::LoopCheckPodInfos() {
    while (true) {
        {
        MutexLock lock(&infos_mutex_);
        for (PodInfosType::iterator it = pod_infos_.begin(); 
             it != pod_infos_.end(); ++it) {
            boost::shared_ptr<PodInfo>& info = it->second;

            boost::shared_ptr<InitdHandler> handler;
            {
            MutexLock lock(&handlers_mutex_);
            handler = initd_handlers_[it->first];
            }

            if (handler->GetStatus() == -1) {
                LOG(INFO, "status -1");
                // initd startup
                info->status.set_state(kPodPending);
                continue;
            }

            if (info->status.state() == kPodPending) {
                const PodDesc& pod = info->desc;
                int ret = 0;
                for (int i = 0; i < pod.desc.tasks_size(); ++i) {
                    std::string task_id;
                    TaskDesc task_desc;
                    task_desc.task.CopyFrom(pod.desc.tasks(i));
                    task_desc.initd_port = handler->GetPort();
                    task_desc.podid = pod.id;
                    ret = task_manager_->CreateTask(task_desc, &task_id);
                    if (ret != 0) {
                        LOG(WARNING, "create pod[%s] task error[%s]", 
                            pod.id.c_str(), task_id.c_str());
                        break;
                    }
                    info->tasksid.push_back(task_id);
                }

                if (ret == 0) {
                    LOG(INFO, "create pod[%s] success", pod.id.c_str());
                    info->status.set_state(kPodDeploy);
                }
            }
        }
        }
        // TODO parameter timeout
        sleep(1);
    }
}

}
}
