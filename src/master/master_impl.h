// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef BAIDU_GALAXY_MASTER_IMPL_H
#define BAIDU_GALAXY_MASTER_IMPL_H

#include "proto/master.pb.h"
#include "job_manager.h"
#include "ins_sdk.h"

using ::galaxy::ins::sdk::InsSDK;

namespace baidu {
namespace galaxy {
class MasterImpl : public Master {
public:
      MasterImpl();
      virtual ~MasterImpl();
      void AcquireMasterLock();
      void Init();
      void ReloadJobInfo();
      virtual void SubmitJob(::google::protobuf::RpcController* controller,
                            const ::baidu::galaxy::SubmitJobRequest* request,
                            ::baidu::galaxy::SubmitJobResponse* response,
                            ::google::protobuf::Closure* done);
      virtual void UpdateJob(::google::protobuf::RpcController* controller,
                            const ::baidu::galaxy::UpdateJobRequest* request,
                            ::baidu::galaxy::UpdateJobResponse* response,
                            ::google::protobuf::Closure* done);
      virtual void SuspendJob(::google::protobuf::RpcController* controller,
                            const ::baidu::galaxy::SuspendJobRequest* request,
                            ::baidu::galaxy::SuspendJobResponse* response,
                            ::google::protobuf::Closure* done);
      virtual void ResumeJob(::google::protobuf::RpcController* controller,
                            const ::baidu::galaxy::ResumeJobRequest* request,
                            ::baidu::galaxy::ResumeJobResponse* response,
                            ::google::protobuf::Closure* done);
      virtual void TerminateJob(::google::protobuf::RpcController* controller,
                                const ::baidu::galaxy::TerminateJobRequest* request,
                                ::baidu::galaxy::TerminateJobResponse* response,
                                ::google::protobuf::Closure* done);
      virtual void ShowJob(::google::protobuf::RpcController* controller,
                           const ::baidu::galaxy::ShowJobRequest* request,
                           ::baidu::galaxy::ShowJobResponse* response,
                           ::google::protobuf::Closure* done);
      virtual void ListJobs(::google::protobuf::RpcController* controller,
                            const ::baidu::galaxy::ListJobsRequest* request,
                            ::baidu::galaxy::ListJobsResponse* response,
                            ::google::protobuf::Closure* done);
      virtual void HeartBeat(::google::protobuf::RpcController* controller,
                             const ::baidu::galaxy::HeartBeatRequest* request,
                             ::baidu::galaxy::HeartBeatResponse* response,
                             ::google::protobuf::Closure* done);
      virtual void GetPendingJobs(::google::protobuf::RpcController* controller,
                                  const ::baidu::galaxy::GetPendingJobsRequest* request,
                                  ::baidu::galaxy::GetPendingJobsResponse* response,
                                  ::google::protobuf::Closure* done);
      virtual void GetResourceSnapshot(::google::protobuf::RpcController* controller,
                                       const ::baidu::galaxy::GetResourceSnapshotRequest* request,
                                       ::baidu::galaxy::GetResourceSnapshotResponse* response,
                                       ::google::protobuf::Closure* done);
      virtual void Propose(::google::protobuf::RpcController* controller,
                           const ::baidu::galaxy::ProposeRequest* request,
                           ::baidu::galaxy::ProposeResponse* response,
                           ::google::protobuf::Closure* done);
      virtual void ListAgents(::google::protobuf::RpcController* controller,
                              const ::baidu::galaxy::ListAgentsRequest* request,
                              ::baidu::galaxy::ListAgentsResponse* response,
                              ::google::protobuf::Closure* done);
      void OnSessionTimeout();
      void OnLockChange(std::string lock_session_id);
private:
      JobManager job_manager_;
      InsSDK* nexus_;
};

}
}

#endif
