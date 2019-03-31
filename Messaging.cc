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

#include "Messaging.hh"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/graph/graph_utility.hpp>
#include <chrono>
#include <iostream>
#include <thread>

#include "Controller.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"
#include "Switch.hh"


REGISTER_APPLICATION(Messaging, {"controller", ""})


using namespace runos;
using namespace std;


void Messaging::onLinkDiscovered(switch_and_port from, switch_and_port to)
{
  raft::RPC::ClientRequest request;
  request.client_id = peers[myidx].id;
  request.message_id = std::to_string(++message_id);

  request.data = boost::lexical_cast<std::string>(request.client_id) + "," +
                 boost::lexical_cast<std::string>(request.message_id) + "," +
                 std::string("linkDiscovered") + "," +
                 boost::lexical_cast<std::string>(from.dpid) + "," +
                 boost::lexical_cast<std::string>(from.port) + "," +
                 boost::lexical_cast<std::string>(to.dpid) + "," +
                 boost::lexical_cast<std::string>(to.port);

  request_queue_mutex.lock();
  request_queue.push(request);
  request_queue_mutex.unlock();
}

void Messaging::onLinkBroken(switch_and_port from, switch_and_port to)
{
  raft::RPC::ClientRequest request;
  request.client_id = peers[myidx].id;
  request.message_id = std::to_string(++message_id);

  request.data = boost::lexical_cast<std::string>(request.client_id) + "," +
                 boost::lexical_cast<std::string>(request.message_id) + "," +
                 std::string("linkBroken") + "," +
                 boost::lexical_cast<std::string>(from.dpid) + "," +
                 boost::lexical_cast<std::string>(from.port) + "," +
                 boost::lexical_cast<std::string>(to.dpid) + "," +
                 boost::lexical_cast<std::string>(to.port);

  request_queue_mutex.lock();
  request_queue.push(request);
  request_queue_mutex.unlock();
}


void Messaging::onHostDiscovered(Host* dev)
{
    if(dev->ip() == "0.0.0.0") //TODO: bad logic, all ip's are like this
    {
      LOG(INFO) << "Bad host IP, ignoring" << endl;
      return;
    }

    raft::RPC::ClientRequest request;
    request.client_id = peers[myidx].id;
    request.message_id = std::to_string(++message_id);

    request.data =  boost::lexical_cast<std::string>(request.client_id) + "," +
                    boost::lexical_cast<std::string>(request.message_id) + "," +
                    std::string("hostDiscovered") + "," +
                    boost::lexical_cast<std::string>(dev->id()) + "," + 
                    dev->mac() + "," + 
                    boost::lexical_cast<std::string>(dev->switchID())+ "," + 
                    boost::lexical_cast<std::string>(dev->switchPort())+ "," + 
                    dev->ip();

    request_queue_mutex.lock();
    request_queue.push(request);
    request_queue_mutex.unlock();
}

void Messaging::RequestQueueProcessorThread()
{
  //we can only pop from queue is the request is commited
  //we pop in process_events()
  for(;;)
  {
    //not checking if we are leader
    auto *leader = server->raft_server().state.find_peer(server->raft_server().state.leader_id);
    if(!request_queue.empty() && leader != nullptr)
    {
      server->raft_server().on(peers[myidx].id, request_queue.front());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  }
}

void Messaging::init(Loader *loader, const Config &config)
{
    Controller* ctrl = Controller::get(loader);
    auto app_config = config_cd(config, "messaging");
    myidx = config_get(app_config, "myidx", 0);
    heartbeat_ms = config_get(app_config, "heartbeat", 200);

    QObject* ld = ILinkDiscovery::get(loader);
    QObject* hm = HostManager::get(loader);    

    //subscribing to events
    QObject::connect(hm, SIGNAL(hostDiscovered(Host*)), 
                     this, SLOT(onHostDiscovered(Host*)));
    QObject::connect(ld, SIGNAL(linkDiscovered(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkDiscovered(switch_and_port, switch_and_port)));
    QObject::connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkBroken(switch_and_port, switch_and_port)));

    std::string id;
    std::string ip;
    int port;
    int client_port;

    auto jpeers = app_config.at("peers");
    for (auto& jpeer : jpeers.array_items())
    {
      Config cc = jpeer.object_items();
      id = config_get(cc, "name", "");
      ip = config_get(cc, "ip", "");
      port = config_get(cc, "port", 0);
      client_port = config_get(cc, "cport", 0);
      raft::peer_info_t peer(id, ip, port, client_port);
      peers.push_back(peer);
    }
}


void Messaging::process_host_discovered(std::vector<string>& tokens)
{
  //if host's mac already in hosts, delete host from hosts, add new host to hosts
  Host newhost(tokens[4], IPv4Addr(tokens[7]));
  newhost.m->id = std::stoull(tokens[3]);
  newhost.m->switchID = std::stoull(tokens[5]);
  newhost.m->switchPort = std::stoul(tokens[6]);

  m->hosts.push_back(newhost);
}

void Messaging::process_link_discovered(std::vector<string>& tokens)
{
    switch_and_port from = {std::stoull(tokens[3]), std::stoul(tokens[4])}; 
    switch_and_port to = {std::stoull(tokens[5]), std::stoul(tokens[6])};

    std::cout << "from.dpid: " << from.dpid << std::endl;
    std::cout << "from.port: " << from.port << std::endl;

    std::cout << "to.dpid: " << to.dpid << std::endl;
    std::cout << "to.port: " << to.port << std::endl;


    if (from.dpid == to.dpid) {
        LOG(WARNING) << "Ignoring loopback link on " << from.dpid;
        return;
    }
    QWriteLocker locker(&m->graph_mutex);
    auto u = m->vertex(from.dpid);
    auto v = m->vertex(to.dpid);
    add_edge(u, v, link_property{from, to, 1}, m->graph);

    //Link* link = new Link(from, to, 5, rand()%1000 + 2000);
    //topo.push_back(link);
    //addEvent(Event::Add, link);
    
    boost::print_graph(m->graph);
}

void Messaging::process_link_broken(std::vector<string>& tokens)
{
    switch_and_port from = {std::stoull(tokens[3]), std::stoul(tokens[4])}; 
    switch_and_port to = {std::stoull(tokens[5]), std::stoul(tokens[6])};

    QWriteLocker locker(&m->graph_mutex);
    remove_edge(m->vertex(from.dpid), m->vertex(to.dpid), m->graph);

    //Link* link = getLink(from, to);
    //addEvent(Event::Delete, link);
    //topo.erase(std::remove(topo.begin(), topo.end(), link), topo.end());
    boost::print_graph(m->graph);
}

void Messaging::process_entries(std::vector<raft::Entry<std::string>>& entries)
{
  //TODO: implement process logic
  for(int i = 0; i < entries.size(); i++)
  {    
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, entries[i].data, boost::is_any_of(","));

        if(tokens[2] == std::string("hostDiscovered"))
          process_host_discovered(tokens);
        else if(tokens[2] == std::string("linkDiscovered"))
          process_link_discovered(tokens);
        else if(tokens[2] == std::string("linkBroken"))
          process_link_broken(tokens);
        else
          LOG(INFO) << "Unidentified event received, ignoring" << std::endl;

        std::cout << "RECEIVED EVENT:" << std::endl;
        for(int j = 0; j < tokens.size(); j++)
        {
          cout << tokens[j] << endl;
        }

        if(tokens[0] == boost::lexical_cast<std::string>(request_queue.front().client_id) &&
           tokens[1] == boost::lexical_cast<std::string>(request_queue.front().message_id))
        {
          request_queue_mutex.lock();
          std::cout << "Entry commited: popping " << entries[i].data << "from request_queue" << std::endl;
          request_queue.pop();
          request_queue_mutex.unlock();
        }
        /*
        vector<string> substrings = split(entries[i].data.c_str());
        IPv4Addr ip("0.0.0.0");
        Host temp(substrings[2], ip);
        temp.ip(substrings[5]);
        temp.switchID(boost::lexical_cast<uint64_t>(substrings[3].c_str()));
        temp.switchPort(boost::lexical_cast<uint64_t>(substrings[4].c_str()));
        commited_hosts.push_back(temp);
        LOG(INFO) << "New host attached to global host vision, IP: " << temp.ip() << " MAC: " << temp.mac();
        */ 
  }
}


void Messaging::RaftThread()
{
  asio::io_service io_service;
  avery::MyMessageProcessoryFactory message_factory;
  server = std::make_shared<network::asio::Server>(io_service, peers[myidx].ip_port.port, peers[myidx].ip_port.client_port, peers[myidx].id, peers, std::unique_ptr<raft::Storage<std::string> >{ new avery::MemStorage(std::string(peers[myidx].id + ".log").c_str()) }, message_factory, heartbeat_ms);
  server->start();
  io_service.run();
}

void Messaging::StorageCheckerThread()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for(;;)
  {
    raft_log_commited_idx = server->raft_server().storage->log_state().commit.index;
    if(raft_log_commited_idx > local_commited_idx)
    {
      std::vector<raft::Entry<std::string>> entries = server->raft_server().storage->entries_since(local_commited_idx);
      /* TODO: consider possibility to have several events in one commit */
      process_entries(entries);
      local_commited_idx = raft_log_commited_idx;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); //TODO: checker thread remove timeout or config
  }
}

void Messaging::startUp(Loader *loader)
{;
    
    std::thread t1(&Messaging::RaftThread,this);
    t1.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::thread t2(&Messaging::StorageCheckerThread,this);
    t2.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::thread t3(&Messaging::RequestQueueProcessorThread, this);
    t3.detach();
    //t1.join();
}

