cmake_minimum_required (VERSION 2.8)

project (Interpreter)

aux_source_directory(. SRC_LIST)

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")
add_executable(Interpreter ${SRC_LIST})
find_package(Threads)
target_link_libraries(Interpreter ${CMAKE_THREAD_LIBS_INIT})


# ��ʽָ��������
set(CMAKE_CXX_COMPILER "g++")

 
# ����������Ϣ
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -g -O3 -std=c++17")