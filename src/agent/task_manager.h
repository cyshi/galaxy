#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <map>
#include <vector>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "proto/galaxy.pb.h"
#include "proto/initd.pb.h"
#include "mutex.h"
#include "thread.h"
#include "rpc/rpc_client.h"
#include "pod_info.h"

namespace baidu {

namespace common {
class Thread;
}

namespace galaxy {

struct TaskDesc {
    TaskDescriptor task;
    int initd_port;
    std::string podid;
};

struct TaskInfo {
    // meta infomantion
    TaskDesc desc;

    // runtim information
    ProcessInfo process;
    TaskStatus status;
    std::string cgroup_path;

    uint32_t millicores;

    std::string work_dir;

    std::string taskid;
};

class TaskManager {
public:
    TaskManager();

    ~TaskManager();

    int CreateTask(const TaskDesc& task, std::string* taskid);

    int DeleteTask(const std::string& taskid);

    int UpdateTaskCpuLimit(const std::string& taskid, 
                           const uint32_t millicores);

    // copy from
    int QueryTask(const std::string& taskid, TaskInfo* tasks);
private:

    int Execute(const std::string& command);

    int Kill(const std::string& taskid);

    // int Show(boost::shared_ptr<TaskInfo>& info);

    void LoopCheckTaskStatus();

    std::string GenerateTaskId(const std::string& podid);

    int PrepareWorkspace(boost::shared_ptr<TaskInfo>& task);

    int PrepareCgroupEnv(const boost::shared_ptr<TaskInfo>& task);

    int PrepareVolumeEnv(const boost::shared_ptr<TaskInfo>& task);

    template <class Request, class Response>
    void SendRequestToInitd(
        void(Initd_Stub::*func)(google::protobuf::RpcController*, 
                                const Request*, Response*, 
                                ::google::protobuf::Closure*), 
        const Request* request, 
        Response* response, int port);

    template <class Request, class Response>
    void InitdCallback(const Request* request, 
                       Response* response, 
                       bool failed, int error);
private:

    Mutex tasks_mutex_;

    typedef std::map<std::string, boost::shared_ptr<TaskInfo> > TasksType;
    // key taskid 
    TasksType tasks_;
    // Thread  
    boost::scoped_ptr<common::Thread> monitor_thread_;

    boost::scoped_ptr<RpcClient> rpc_client_;
};

}
}

#endif
