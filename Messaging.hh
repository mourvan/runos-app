/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Application.hh"
#include "Loader.hh"

#include <string>
#include <vector>
#include <queue>
#include <set>
#include <mutex>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "HostManager.hh"
#include "Topology.hh"

#include "raft/raft.hpp"
#include "raft/asio_tcp_server.hpp"
#include "raft/asio_tcp_session.hpp"
#include "raft/mem_storage.hpp"
#include "raft/network_message_processor.hpp"
#include "raft/simple_message_processor.hpp"
#include "raft/simple_serialize.hpp"

using namespace topology;

struct HostImpl {
    uint64_t id;
    std::string mac;
    IPv4Addr ip;
    uint64_t switchID;
    uint32_t switchPort;
};

typedef TopologyGraph::vertex_descriptor
    vertex_descriptor;

struct TopologyImpl //same as in Topology.cc
{
    QReadWriteLock graph_mutex;

    std::vector<Host> hosts;
    TopologyGraph graph;
    std::unordered_map<uint64_t, vertex_descriptor> vertex_map;

    vertex_descriptor vertex(uint64_t dpid) {
    	std::cout << "here 1" << std::endl;
        auto it = vertex_map.find(dpid);
    	std::cout << "here 2" << std::endl;

        if (it != vertex_map.end()) {
    		std::cout << "here 3" << std::endl;
            auto v = it->second;
    		std::cout << "here 4" << std::endl;
            BOOST_ASSERT(get(dpid_t(), graph, v) == dpid);
    		std::cout << "here 5" << std::endl;
            return v;
        } else {
    		std::cout << "here 6" << std::endl;
            auto v = vertex_map[dpid] = add_vertex(graph);
            std::cout << "here 7" << std::endl;
            put(dpid_t(), graph, v, dpid);
    		std::cout << "here 8" << std::endl;
            return v;
        }
    }
};


class Messaging : public Application {
Q_OBJECT
SIMPLE_APPLICATION(Messaging, "messaging")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader *loader) override;

    // network topology information, can be accessed and used by other appications
    TopologyImpl* m;
    
private:
	void process_entries(std::vector<raft::Entry<std::string>>& entries);
	void process_host_discovered(std::vector<std::string>& tokens);
	void process_link_discovered(std::vector<std::string>& tokens);
	void process_link_broken(std::vector<std::string>& tokens);

	void RaftThread();
	void StorageCheckerThread();
	void RequestQueueProcessorThread();

	//raft stuff
	std::queue<raft::RPC::ClientRequest> request_queue;
	std::mutex request_queue_mutex;
	raft::PeerInfo peers;
	std::shared_ptr<network::asio::Server> server;
	int message_id = 0;
	int local_commited_idx = 0;
	int raft_log_commited_idx = 0;

	//config stuff
	int heartbeat_ms;
	int myidx;

public slots:
	void onHostDiscovered(Host* dev);								  //host manager
	void onLinkDiscovered(switch_and_port from, switch_and_port to);  //link discovery
    void onLinkBroken(switch_and_port from, switch_and_port to);      //link discovery
};