cmake_minimum_required(VERSION 3.13)
project(sixelping_receiver_dpdk)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(PNG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
include(FetchContent)


set(ENABLE_TESTING NO)

FetchContent_Declare(
	prometheus-cpp
	GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
	GIT_TAG        v0.8.0
)

# CMake 3.14+
FetchContent_MakeAvailable(prometheus-cpp)

set(PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/src/sixelping-command.proto)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wno-unused-parameter -ggdb -g -march=native")
set(RTE_SDK /usr/share/dpdk)
set(RTE_TARGET x86_64-default-linux-gcc)

add_executable(sixelping_receiver_dpdk ${PROTO_SRCS} ${GRPC_SRCS} src/main.cpp include/icmp6.h include/common.h include/application_configuration.h src/ethdev.cpp include/ethdev.h src/packets.cpp include/packets.h src/pixels.cpp include/pixels.h src/grpc-helper.cpp include/grpc-helper.h src/lcoremain.cpp include/lcoremain.h src/pktmbuf.c src/prometheus.cpp include/prometheus.h)

target_include_directories(sixelping_receiver_dpdk PUBLIC ${RTE_SDK}/${RTE_TARGET}/include /usr/include/dpdk/ /usr/include/x86_64-linux-gnu/dpdk/)
target_link_directories(sixelping_receiver_dpdk PUBLIC ${RTE_SDK}/${RTE_TARGET}/lib)

SET(DPDK_LIBS "-Wl,--whole-archive -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_rcu -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_ring -lrte_eal -lrte_kvargs -lbsd -Wl,--no-whole-archive")

target_include_directories(sixelping_receiver_dpdk PUBLIC include/)
target_include_directories(sixelping_receiver_dpdk PUBLIC ${PROTO_SRC_DIR})
target_link_libraries(sixelping_receiver_dpdk ${DPDK_LIBS})
target_link_libraries(sixelping_receiver_dpdk pthread dl rt m numa)
target_link_libraries(sixelping_receiver_dpdk PNG::PNG)
target_link_libraries(sixelping_receiver_dpdk gRPC::grpc++_reflection protobuf::libprotobuf)
target_link_libraries(sixelping_receiver_dpdk prometheus-cpp::core)
target_link_libraries(sixelping_receiver_dpdk prometheus-cpp::pull)
set_target_properties(sixelping_receiver_dpdk PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS})
