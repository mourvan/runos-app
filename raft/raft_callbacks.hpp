#pragma once
#include <raft_rpc.hpp>

namespace raft {

template <typename data_t>
class Callbacks {
 public:
  virtual void set_heartbeat_timeout() = 0;
  virtual void set_follower_timeout() = 0;
  virtual void set_candidate_timeout() = 0;
  virtual void set_minimum_timeout() = 0;

  virtual void send(const std::string& peer_id,
                    const raft::RPC::AppendEntriesRequest<data_t>& request) = 0;
  virtual void send(const std::string& peer_id,
                    const raft::RPC::AppendEntriesResponse& response) = 0;
  virtual void send(const std::string& peer_id,
                    const raft::RPC::VoteRequest& request) = 0;
  virtual void send(const std::string& peer_id,
                    const raft::RPC::VoteResponse& response) = 0;
  virtual void send(const std::string& peer_id,
                    const raft::RPC::ClientRequest& request) = 0;
  virtual void client_send(const std::string& peer_id,
                    const raft::RPC::ClientResponse& response) = 0;
  virtual void client_send(const std::string& peer_id,
                           const raft::RPC::NotLeaderResponse& response) = 0;
  virtual void client_send(const std::string& peer_id,
                           const raft::RPC::CurrentEntryResponse& response) = 0;

  virtual bool identify(const std::string& temp_id, const std::string& id) = 0;
  virtual void drop(const std::string& peer_id) = 0;

  virtual void new_leader_elected(const std::string &id) = 0;
  virtual void commit_advanced(uint64_t commit_index) = 0;
  inline virtual ~Callbacks() {}
};
}
