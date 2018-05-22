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
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include <string>
#include "Controller.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"

REGISTER_APPLICATION(Messaging, {"controller", ""})


using namespace runos;
using namespace std;



void Messaging::onHostDiscovered(Host* dev)
{ 
    //когда контроллер находит новый хост, уведомляем рафт-кластер о событии
    //обработка новых хостов остальными серверами происходит при десериализации сообщения
    raft::RPC::ClientRequest request;
    request.client_id = peers[myidx].id;
    message_id++;
    request.message_id = message_id;

    request.data = "hostDiscovered " + boost::lexical_cast<std::string>(dev->id()) + " " + dev->mac()+ " " + boost::lexical_cast<std::string>(dev->switchID())+ " " + boost::lexical_cast<std::string>(dev->switchPort())+ " " + dev->ip();
    //cout << request.data << endl; 
   // cout << "hostDiscovered signal received!!!" << endl;
    server->raft_server().on(peers[myidx].id, request);

    //когда эта энтри - коммитед - добавляем ее в вектор хостов
}

void Messaging::init(Loader *loader, const Config &config)
{
    Controller* ctrl = Controller::get(loader);
    auto app_config = config_cd(config, "messaging");
    myidx = config_get(app_config, "myidx", 0);
    heartbeat_ms = config_get(app_config, "heartbeat", 200);
    m_host_manager = HostManager::get(loader);
    QObject::connect(m_host_manager, &HostManager::hostDiscovered, this, &Messaging::onHostDiscovered);

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


/* - CORNERSTONE
void run_raft_instance_with_asio(int srv_id) {
    ptr<logger> l(asio_svc_->create_logger(asio_service::log_level::debug, sstrfmt("log%d.log").fmt(srv_id)));
    ptr<rpc_listener> listener(asio_svc_->create_rpc_listener((ushort)(9000 + srv_id), l));
    ptr<state_mgr> smgr(cs_new<simple_state_mgr>(srv_id));
    ptr<state_machine> smachine(cs_new<echo_state_machine>());
    raft_params* params(new raft_params());
    (*params).with_election_timeout_lower(200)
        .with_election_timeout_upper(400)
        .with_hb_interval(100)
        .with_max_append_size(100)
        .with_rpc_failure_backoff(50);
    ptr<delayed_task_scheduler> scheduler = asio_svc_;
    ptr<rpc_client_factory> rpc_cli_factory = asio_svc_;
    context* ctx(new context(smgr, smachine, listener, l, rpc_cli_factory, scheduler, params));
    ptr<raft_server> server(cs_new<raft_server>(ctx));
    listener->listen(server);

    {
        std::unique_lock<std::mutex> ulock(lock1);
        stop_cv1.wait(ulock);
        listener->stop();
    }
}
*/


/*
using namespace boost::asio;

void client_thread()
{
  io_service ios; 
  ip::udp::endpoint broadcast_endpoint(ip::address_v4::broadcast(), _port1); //where to send info
  ip::udp::socket sock(ios, ip::udp::endpoint(ip::udp::v4(), 0)); 
  sock.set_option(ip::udp::socket::reuse_address(true));
  sock.set_option(socket_base::broadcast(true));
  for(;;)
  {
    string msg;
    cin >> msg;
    sock.send_to(buffer(msg), broadcast_endpoint);
  }
}

void server_thread()
{
  io_service ios;  
  string msg;
  ip::udp::socket sock(ios, ip::udp::endpoint(ip::udp::v4(), _port2)); 
  //sock.set_option(ip::udp::socket::reuse_address(true));
  //sock.set_option(socket_base::broadcast(true));
  //std::array<char, 128> buf;
  for(;;)
  {
    std::array<char, 128> buf;
    ip::udp::endpoint receive_endpoint;
    size_t len = sock.receive_from(buffer(buf), receive_endpoint);
    cout.write(buf.data(), len);
    cout << endl;
  }
}

*/

#include <thread>

vector<string> split(const char *str, char c = ' ')
{
    vector<string> result;

    do
    {
        const char *begin = str;

        while(*str != c && *str)
            str++;

        result.push_back(string(begin, str));
    } 
    while (0 != *str++);

    return result;
}

void Messaging::RaftThread()
{
  asio::io_service io_service;
  avery::MyMessageProcessoryFactory message_factory;
  server = std::make_shared<network::asio::Server>(io_service, peers[myidx].ip_port.port, peers[myidx].ip_port.client_port, peers[myidx].id, peers, std::unique_ptr<raft::Storage<std::string> >{ new avery::MemStorage(std::string(peers[myidx].id + ".log").c_str()) }, message_factory, heartbeat_ms);
  server->start();
  io_service.run();
}

void Messaging::StorageCheckerThread() //TODO: use condition variable!!
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for(;;)
  {
    int storage_idx = server->raft_server().storage->log_state().commit.index;
    if(storage_idx > commited_idx)
    {
      std::vector<raft::Entry<std::string>> entries = server->raft_server().storage->entries_since(commited_idx);

      for(int i = 0; i < (storage_idx - commited_idx); i++)
      {
        LOG(INFO) << "New host attached to global host vision, " << entries[i].data;
        /*
        //need to append hosts to local host vector;- deserialize, append
        if(entries[i].data[0] == ' ') entries[i].data.erase(0,1);
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

      commited_idx = storage_idx;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //pause for not too long
  }
}

void Messaging::startUp(Loader *loader)
{
    //io_service ios;        
    //ip::udp::endpoint broadcast_endpoint(ip::address_v4::broadcast(), port1); //where to send info
    //ip::udp::socket sock(ios, ip::udp::endpoint(ip::udp::v4(), port2)); 
    //sock.set_option(ip::udp::socket::reuse_address(true));
    //sock.set_option(socket_base::broadcast(true));
    
    std::thread t1(&Messaging::RaftThread,this);
    t1.detach();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::thread t2(&Messaging::StorageCheckerThread,this);
    t2.detach();
    //t1.join();

   // for(;;)
   // {
   //     ip::udp::endpoint receive_endpoint;
   //     sock.async_receive_from(buffer(buf), receive_endpoint, boost::bind(&Messaging::read_handler,this, boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
   //     ios.run();
   // }


  // по сети гуляют voteRequestы!!!!
  // ACKи проходят, значит доставка есть, видимо плохо работает обработка входящих пакетов - она их просто не видит!
  // voteRequest отправляется на непонятный порт и непонятно, что с него возвращается кроме ACk
  // проблема в asio_tcp_server или в asio_tcp_session
  // не понимаю: где ошибка
  // не отправляет voteResponse!!!! сервера получают сообщения друг от друга: но не отправляют обратно ответы
  // simple_message_processor - мб тут ошибка? нет реализации процессирующей сообщения!!
  // походу все таки ошибка в raft_server.ipp
  // закомментил пару строчек в raft_server.ipp - теперь даже выбирает лидера, но почему то произвольно меняются этапы!!
  // при этом лог синхронизируется между всеми, лог консистентный, но каждый раз меняет лидера
  // исправил невыбираемого лидера, теперь почему то нелидер отправляет AppednEntries
  // получает хартбит, но таймер не обнуляется???
  // возможно потому что выкидывает сразу все пакеты (heartbeat) одновременно
  // + не понимаю, как выполнять clent request

  //пофиксил heartbeat, но он почему то все равно продолжает сбрасывать лидерство

  /*

  VoteRequest received
AppendEntriesRequest received, current leader: - нету лидера
// неправлильно пишет лидера после выигранного голосования!!!
// мб проблема в on(appendentriesrequest), что он не обновляет лидера


// он получает heartbeat, при этом инициирует голосование!! они не могут согласиться на лидере!!
// сервер локально не принимает лидера!!

//appendEntriesResponse отправляется6 НО НЕ ПРИНИМАЕТСЯ




//мы не аппендим энтрис в storage!!!!!!!


  */
}

