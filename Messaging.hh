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
#include "HostManager.hh"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
//#include "cornerstone/include/cornerstone.hxx"
#include "raft/raft.hpp"
#include "raft/asio_tcp_server.hpp"
#include "raft/asio_tcp_session.hpp"
#include "raft/mem_storage.hpp"
#include "raft/network_message_processor.hpp"
#include "raft/simple_message_processor.hpp"
#include "raft/simple_serialize.hpp"



class Messaging : public Application {
Q_OBJECT
SIMPLE_APPLICATION(Messaging, "messaging")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader *loader) override;

    std::vector<Host> commited_hosts;
private:
	void write_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void read_handler(const boost::system::error_code& ec,std::size_t bytes_transferred);
	void RaftThread();
	void StorageCheckerThread();
	std::string type;
	//std::string ip;
	//int port1,port2;
	HostManager* m_host_manager;
	//std::array<char, 128> buf;
	raft::PeerInfo peers;
	int heartbeat_ms;
	int myidx;
	std::shared_ptr<network::asio::Server> server;
	int message_id = 0;
	int commited_idx = 0;


public slots:
	void onHostDiscovered(Host* dev);
};