#ifndef INITDHANDLER_H
#define INITDHANDLER_H

#include <sofa/pbrpc/pbrpc.h>
#include <boost/atomic.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "rpc/rpc_client.h"
#include "proto/initd.pb.h"
#include "pod_info.h"

namespace baidu {

namespace common {
class Thread;
}

namespace galaxy  {

class PodInfo;
class ExecuteRequest;
class ExecuteResponse;

class InitdHandler {
public:
    InitdHandler();

    ~InitdHandler();

    int Execute(const std::string& command);
    
    int Create(const std::string& podid, 
               const std::string& work_dir);

    int GetStatus();

    int GetPort() {
        return port_;
    }

    // int Delete();

    // int UpdateCpuLimit(const Resource& usage);

    // int Show(boost::shared_ptr<PodInfo>& info);

private:
    void CheckPodInfo();

    template <class Request, class Response>
    void SendRequestToInitd(
        void(Initd_Stub::*func)(google::protobuf::RpcController*, 
                                const Request*, Response*, 
                                ::google::protobuf::Closure*), 
        const Request* request, 
        Response* response);


    template <class Request, class Response>
    void InitdCallback(const Request* request, 
                       Response* response, 
                       bool failed, int error);
private:
    std::string podid_;

    // rpc thread create & backgroud loop thread
    Mutex info_mutex_;
    PodInfo pod_info_;

    boost::scoped_ptr<RpcClient> rpc_client_;

    // initd listen port
    int port_;

    boost::atomic<int> status_;
};

}
}

#endif
