#include "task_manager.h"
#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include "gflags/gflags.h"
#include "logging.h"
#include "agent/utils.h"

DECLARE_string(gce_work_dir);
DECLARE_int32(agent_rpc_initd_timeout);
DECLARE_int32(agent_monitor_tasks_interval);

namespace baidu {
namespace galaxy {

TaskManager::TaskManager() {
    rpc_client_.reset(new RpcClient());

    monitor_thread_.reset(new common::Thread());
    monitor_thread_->Start(
        boost::bind(&TaskManager::LoopCheckTaskStatus, this));
}

TaskManager::~TaskManager() {
}

int TaskManager::CreateTask(const TaskDesc& task, std::string* taskid) {
    const TaskDescriptor& task_desc = task.task;
    boost::shared_ptr<TaskInfo> task_info(new TaskInfo());
    *taskid = GenerateTaskId(task.podid);
    task_info->desc.task.CopyFrom(task_desc);
    task_info->desc.initd_port = task.initd_port;
    // task_info->desc.root_dir = task.root_dir;
    task_info->desc.podid = task.podid;
    task_info->status.set_state(kPodPending);
    task_info->millicores = task_desc.requirement().millicores();
    task_info->taskid = *taskid;

    // 1. prepare workspace 
    if (PrepareWorkspace(task_info) != 0) {
        LOG(WARNING, "prepare task %s workspace failed", 
            taskid->c_str()); 
        return -1;
    }
    // 2. prepare cgroup 
    if (PrepareCgroupEnv(task_info) != 0) {
        LOG(WARNING, "prepare task %s cgroup faield", 
            taskid->c_str()); 
        return -1;
    }
    // 3. prepare mount path
    if (PrepareVolumeEnv(task_info) != 0) {
        LOG(WARNING, "prepare task %s volume env failed",
            taskid->c_str()); 
        return -1;
    }

    LOG(INFO, "prepare task[%s] success, work_dir[%s]", 
        taskid->c_str(), task_info->work_dir.c_str());

    // Execute
    ExecuteRequest* request = new ExecuteRequest();
    ExecuteResponse* response = new ExecuteResponse();

    request->set_key(*taskid);
    request->set_commands(task.task.start_command());

    request->set_path(task_info->work_dir);

    SendRequestToInitd(&Initd_Stub::Execute, 
                       request, response, task.initd_port);
    LOG(INFO, "send request to initd[%d] command[%s]", 
        task.initd_port, task.task.start_command().c_str());

    {
    // update tasks
    MutexLock lock(&tasks_mutex_);
    tasks_[*taskid] = task_info;
    }
    return 0;
}


int TaskManager::DeleteTask(const std::string& taskid) {
    return 0;
}

int TaskManager::UpdateTaskCpuLimit(const std::string& taskid, 
                                    const uint32_t millicores) {
    return 0;
}

int QueryTask(const std::string& taskid, 
              TaskInfo* tasks) {
    return 0;
}

int TaskManager::Execute(const std::string& command) {
    return 0;
}

// int TaskManager::Update(const std::string& taskid, const uint32_t millicores) {
//     return 0;
// }

std::string TaskManager::GenerateTaskId(const std::string& podid) {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();    
    std::stringstream sm_uuid;
    sm_uuid << uuid;
    std::string str_uuid = podid;
    str_uuid.append("_");
    str_uuid.append(sm_uuid.str());
    return str_uuid;
}

// TODO
int TaskManager::PrepareWorkspace(boost::shared_ptr<TaskInfo>& task) {
    std::string task_workspace("./");
    task_workspace.append(task->desc.podid);
    task_workspace.append("/");
    // LOG(INFO, "task root_dir[%s]", task->desc.root_dir.c_str());
    // if (!file::Mkdir(workspace_root)) {
    //     LOG(WARNING, "mkdir workspace root failed[%s]:[%s, %d]", 
    //         workspace_root.c_str(), strerror(errno), errno); 
    //     // return 0;
    // }

    // task_workspace.append("./");
    task_workspace.append(task->taskid);
    if (!file::Mkdir(FLAGS_gce_work_dir + "/" + task_workspace)) {
        LOG(WARNING, "mkdir task workspace failed[%s]:[%s, %d]", 
            task_workspace.c_str(), strerror(errno), errno);
        // return 0;
    }
    task->work_dir = task_workspace;
    return 0;
}

int TaskManager::PrepareCgroupEnv(const boost::shared_ptr<TaskInfo>& task) {
    // if (task == NULL) {
    //     return -1; 
    // }
    // std::vector<std::string>::iterator hier_it = 
    //     hierarchies_.begin();
    // std::string cgroup_name = task->task_id;
    // for (; hier_it != hierarchies_.end(); ++ hier_it) {
    //     std::string cgroup_dir = *hier_it;
    //     cgroup_dir.append("/");
    //     cgroup_dir.append(cgroup_name);
    //     if (!file::Mkdir(cgroup_dir)) {
    //         LOG(WARNING, "create dir %s failed for %s",
    //                 cgroup_dir.c_str(), task->task_id.c_str()); 
    //         return -1;
    //     }              
    // }
    // TODO set cgroup contro_file
    return 0;
}

int TaskManager::PrepareVolumeEnv(const boost::shared_ptr<TaskInfo>& task) {
    return 0;
}

template <class Request, class Response>
void TaskManager::SendRequestToInitd(
    void(Initd_Stub::*func)(google::protobuf::RpcController*,
                            const Request*, Response*, 
                            ::google::protobuf::Closure*), 
    const Request* request, 
    Response* response, int port) {

    Initd_Stub* initd; 
    std::string endpoint("localhost:");
    endpoint += boost::lexical_cast<std::string>(port);
    rpc_client_->GetStub(endpoint, &initd);

    boost::function<void(const Request*, Response*, bool, int)> callback;
    callback = boost::bind(&TaskManager::InitdCallback<Request, Response>, 
                           this, _1, _2, _3, _4);
    rpc_client_->AsyncRequest(initd, func, request, response, 
                              callback, FLAGS_agent_rpc_initd_timeout, 0);
    delete initd;
}

template <class Request, class Response>
void TaskManager::InitdCallback(const Request* request, 
                                Response* response, 
                                bool failed, int error) {
    boost::scoped_ptr<const Request> ptr_request(request);
    boost::scoped_ptr<Response> ptr_response(response);

    if (failed || error != 0) {
        LOG(WARNING, "initd rpc error[%d]", error);
    }

    return;
}

void TaskManager::LoopCheckTaskStatus() {
    while (true) {
        {
        MutexLock lock(&tasks_mutex_);
        for (TasksType::iterator it = tasks_.begin(); 
             it != tasks_.end(); ++it) {
            LOG(INFO, "get task[%s] status", it->second->taskid.c_str());
            GetProcessStatusRequest* request = new GetProcessStatusRequest();
            GetProcessStatusResponse* response = new GetProcessStatusResponse();

            int port = it->second->desc.initd_port;
            request->set_key(it->first);
            SendRequestToInitd(&Initd_Stub::GetProcessStatus, 
                               request, response, port);
        }
        }
        sleep(FLAGS_agent_monitor_tasks_interval);
    }
}

template <>
void TaskManager::InitdCallback(const GetProcessStatusRequest* request, 
                                GetProcessStatusResponse* response, 
                                bool failed, int error) {

    // LOG(INFO, "get process status callback[%s]", request->key().c_str());

    boost::scoped_ptr<const GetProcessStatusRequest> ptr_request(request);
    boost::scoped_ptr<GetProcessStatusResponse> ptr_response(response);

    if (failed || error != 0) {
        LOG(WARNING, "initd rpc error[%d]", error);
        return;
    }

    LOG(INFO, "process[%s] status[%d]", request->key().c_str(), ptr_response->status());

    return;
}

}
}
