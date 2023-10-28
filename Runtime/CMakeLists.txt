cmake_minimum_required (VERSION 2.8)

project (Runtime)

set(C_SOURCE gloconst.c vm.c vmobj.c main.c hashmap.c syscall.c)
set(C_HEADER gloconst.h vm.h vmobj.h hashmap.h syscall.h)
add_executable(Runtime ${C_SOURCE} ${C_HEADER})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Runtime ${CMAKE_THREAD_LIBS_INIT})
# find_package(Threads)
# target_link_libraries(vm_c ${MAKE_THREAD_LIBS_INIT})
# target_sources(vm_c PUBLIC ${CPP_SOURCE} ${CPP_HEADER})

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")


# set the compiler
set(CMAKE_C_COMPILER "gcc")

 
# set the arguments
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c11 -lpthread")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -std=c11 -lpthread")