# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jhe/Projects/VC/Runtime

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jhe/Projects/VC/build/Runtime

# Include any dependencies generated for this target.
include CMakeFiles/Runtime.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Runtime.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Runtime.dir/flags.make

CMakeFiles/Runtime.dir/gloconst.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/gloconst.c.o: /home/jhe/Projects/VC/Runtime/gloconst.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/Runtime.dir/gloconst.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/gloconst.c.o   -c /home/jhe/Projects/VC/Runtime/gloconst.c

CMakeFiles/Runtime.dir/gloconst.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/gloconst.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/gloconst.c > CMakeFiles/Runtime.dir/gloconst.c.i

CMakeFiles/Runtime.dir/gloconst.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/gloconst.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/gloconst.c -o CMakeFiles/Runtime.dir/gloconst.c.s

CMakeFiles/Runtime.dir/vm.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/vm.c.o: /home/jhe/Projects/VC/Runtime/vm.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/Runtime.dir/vm.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/vm.c.o   -c /home/jhe/Projects/VC/Runtime/vm.c

CMakeFiles/Runtime.dir/vm.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/vm.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/vm.c > CMakeFiles/Runtime.dir/vm.c.i

CMakeFiles/Runtime.dir/vm.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/vm.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/vm.c -o CMakeFiles/Runtime.dir/vm.c.s

CMakeFiles/Runtime.dir/vmobj.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/vmobj.c.o: /home/jhe/Projects/VC/Runtime/vmobj.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/Runtime.dir/vmobj.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/vmobj.c.o   -c /home/jhe/Projects/VC/Runtime/vmobj.c

CMakeFiles/Runtime.dir/vmobj.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/vmobj.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/vmobj.c > CMakeFiles/Runtime.dir/vmobj.c.i

CMakeFiles/Runtime.dir/vmobj.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/vmobj.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/vmobj.c -o CMakeFiles/Runtime.dir/vmobj.c.s

CMakeFiles/Runtime.dir/main.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/main.c.o: /home/jhe/Projects/VC/Runtime/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/Runtime.dir/main.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/main.c.o   -c /home/jhe/Projects/VC/Runtime/main.c

CMakeFiles/Runtime.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/main.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/main.c > CMakeFiles/Runtime.dir/main.c.i

CMakeFiles/Runtime.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/main.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/main.c -o CMakeFiles/Runtime.dir/main.c.s

CMakeFiles/Runtime.dir/hashmap.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/hashmap.c.o: /home/jhe/Projects/VC/Runtime/hashmap.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/Runtime.dir/hashmap.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/hashmap.c.o   -c /home/jhe/Projects/VC/Runtime/hashmap.c

CMakeFiles/Runtime.dir/hashmap.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/hashmap.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/hashmap.c > CMakeFiles/Runtime.dir/hashmap.c.i

CMakeFiles/Runtime.dir/hashmap.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/hashmap.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/hashmap.c -o CMakeFiles/Runtime.dir/hashmap.c.s

CMakeFiles/Runtime.dir/syscall.c.o: CMakeFiles/Runtime.dir/flags.make
CMakeFiles/Runtime.dir/syscall.c.o: /home/jhe/Projects/VC/Runtime/syscall.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/Runtime.dir/syscall.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Runtime.dir/syscall.c.o   -c /home/jhe/Projects/VC/Runtime/syscall.c

CMakeFiles/Runtime.dir/syscall.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Runtime.dir/syscall.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jhe/Projects/VC/Runtime/syscall.c > CMakeFiles/Runtime.dir/syscall.c.i

CMakeFiles/Runtime.dir/syscall.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Runtime.dir/syscall.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jhe/Projects/VC/Runtime/syscall.c -o CMakeFiles/Runtime.dir/syscall.c.s

# Object files for target Runtime
Runtime_OBJECTS = \
"CMakeFiles/Runtime.dir/gloconst.c.o" \
"CMakeFiles/Runtime.dir/vm.c.o" \
"CMakeFiles/Runtime.dir/vmobj.c.o" \
"CMakeFiles/Runtime.dir/main.c.o" \
"CMakeFiles/Runtime.dir/hashmap.c.o" \
"CMakeFiles/Runtime.dir/syscall.c.o"

# External object files for target Runtime
Runtime_EXTERNAL_OBJECTS =

Runtime: CMakeFiles/Runtime.dir/gloconst.c.o
Runtime: CMakeFiles/Runtime.dir/vm.c.o
Runtime: CMakeFiles/Runtime.dir/vmobj.c.o
Runtime: CMakeFiles/Runtime.dir/main.c.o
Runtime: CMakeFiles/Runtime.dir/hashmap.c.o
Runtime: CMakeFiles/Runtime.dir/syscall.c.o
Runtime: CMakeFiles/Runtime.dir/build.make
Runtime: CMakeFiles/Runtime.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jhe/Projects/VC/build/Runtime/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking C executable Runtime"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Runtime.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Runtime.dir/build: Runtime

.PHONY : CMakeFiles/Runtime.dir/build

CMakeFiles/Runtime.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Runtime.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Runtime.dir/clean

CMakeFiles/Runtime.dir/depend:
	cd /home/jhe/Projects/VC/build/Runtime && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jhe/Projects/VC/Runtime /home/jhe/Projects/VC/Runtime /home/jhe/Projects/VC/build/Runtime /home/jhe/Projects/VC/build/Runtime /home/jhe/Projects/VC/build/Runtime/CMakeFiles/Runtime.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Runtime.dir/depend

