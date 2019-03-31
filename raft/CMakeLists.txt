set(CMAKE_AUTOMOC OFF)

set(SOURCES
    asio_tcp_server.cpp
    asio_tcp_server.hpp
    asio_tcp_session.cpp
    asio_tcp_session.hpp
    base64.h
    mem_storage.hpp
    mem_storage.cpp
    network_message_processor.hpp
    raft.hpp
    raft_callbacks.hpp
    raft_entry.hpp
    raft_rpc.hpp
    raft_server.hpp
    raft_server.ipp
    raft_state.hpp
    raft_storage.hpp
    simple_message_processor.hpp
    simple_message_processor.cpp
    simple_serialize.cpp
    simple_serialize.hpp
    
)


include_directories(.)

add_library(runos_raft STATIC ${SOURCES})

target_link_libraries(runos_raft ${Boost_UNIT_TEST_FRAMEWORK})
