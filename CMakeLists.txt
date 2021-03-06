add_subdirectory(types)
add_subdirectory(maple)
add_subdirectory(raft)

set(SOURCES
    # Core
    Application.cc 
    Loader.cc
    Decision.cc
    Flow.cc
    OFMsgUnion.cc
    OFTransaction.cc
    FluidOXMAdapter.cc
    SwitchConnection.cc
    PacketParser.cc
    Controller.cc
    Switch.cc
    LinkDiscovery.cc
    Topology.cc
    STP.cc
    # Apps
    SimpleLearningSwitch.cc
    LearningSwitch.cc
    CBench.cc
    Stats.cc
    ArpHandler.cc
    StaticFlowPusher.cc
    RestMultipart.cc
    RestFlowMod.cc
    RestStringProcessing.cc
    Messaging.cc
    # Loader
    Main.cc
    # REST
    RestListener.cc
    Event.cc
    AppObject.cc
    HostManager.cc
    WebUIManager.cc
    FlowManager.cc
    # Json
    json11.cpp
)

add_definitions(
    -DBOOST_ENABLE_ASSERT_HANDLER
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
    ${CMAKE_SOURCE_DIR}/third_party/webserver
    raft
)

add_executable(runos ${SOURCES}) 
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

FIND_PACKAGE(Boost REQUIRED COMPONENTS system date_time serialization)

target_link_libraries(runos
    runos_types
    runos_raft
    runos_maple
    tcmalloc
    Qt5::Core
    fluid_base
    libfluid_msg.a
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_COROUTINE_LIBRARY}
    ${GLOG_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_LIBRARIES}
    pthread 

)
