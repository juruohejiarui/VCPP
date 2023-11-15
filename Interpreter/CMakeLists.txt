cmake_minimum_required (VERSION 2.8)

project (Interpreter)

set(CXX_SOURCE cpltree.cpp gloconst.cpp idenstring.cpp idensystem.cpp Interpreter.cpp pbuilder.cpp pretreat.cpp vasm.cpp vasmbuilder.cpp vcppcpl.cpp)
set(CXX_HEADER gloconst.h pbuilder.h pretreat.h vasm.h vcppcpl.h)  

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")
add_executable(Interpreter ${CXX_SOURCE} ${CXX_HEADER})
find_package(Threads)
target_link_libraries(Interpreter ${CMAKE_THREAD_LIBS_INIT})


# ��ʽָ��������
set(CMAKE_CXX_COMPILER "g++")

 
# ����������Ϣ
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -g -O3 -std=c++17")