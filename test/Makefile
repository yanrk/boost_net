# arguments
platform = linux/x64



# paths home
project_home       = .
bin_dir            = $(project_home)/bin/$(platform)
object_dir         = $(project_home)/.objs
boost_net_home     = $(project_home)/../..



# includes of boost headers
boost_inc_path     = /usr/local/include/boost_1_88_0
boost_includes     = -I$(boost_inc_path)

# includes of boost_net headers
boost_net_inc_path = $(boost_net_home)/inc
boost_net_includes = -I$(boost_net_inc_path)

# includes of local headers
local_inc_path     = $(project_home)
local_includes     = -I$(local_inc_path)

# all includes that local solution needs
includes           = $(boost_includes)
includes          += $(boost_net_includes)
includes          += $(local_includes)



# source files of local solution
local_src_path     = $(project_home)
local_source       = $(filter %.cpp, $(shell find $(local_src_path) -depth -name "*.cpp"))



# objects of local solution
local_objects      = $(local_source:$(project_home)%.cpp=$(object_dir)%.o)



# system librarys
system_libs        = -lpthread

# boost librarys
boost_lib_inc      = /usr/local/lib
boost_libs         = -L$(boost_lib_inc) -lboost_container -lboost_system -lboost_thread

# boost_net librarys
boost_net_lib_inc  = $(boost_net_home)/lib/$(platform)
boost_net_libs     = -L$(boost_net_lib_inc) -lboost_net

# local depends librarys
depend_libs        = $(system_libs)
depend_libs       += $(boost_libs)
depend_libs       += $(boost_net_libs)



# output execution
output_exec        = $(bin_dir)/boost_net_test_client



# my g++ not support nullptr and 64bits
# so no -m64, and add a macro -Dnullptr=0

# build flags for objects
build_obj_flags    = -std=c++0x -g -Wall -O1 -pipe -fPIC -Dnullptr=0

# build flags for execution
build_exec_flags   = $(build_obj_flags)



# build targets

# let 'build' be default target, build all targets
build    : $(local_objects)
	@echo
	@echo "@@@@@  start making boost_net_test_client  @@@@@"
	g++ $(build_exec_flags) -o $(output_exec) $^ $(depend_libs)
	@echo "@@@@@  make boost_net_test_client success  @@@@@"
	@echo

cpfile   :
	@cp $(boost_net_lib_inc)/* $(bin_dir)/

run      : cpfile
	@export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(bin_dir) && $(output_exec)
# it same as: @env LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(bin_dir) $(output_exec)

# build all objects
$(object_dir)/%.o:$(project_home)/%.cpp
	@dir=`dirname $@`;      \
    if [ ! -d $$dir ]; then \
        mkdir -p $$dir;     \
    fi
	g++ -c $(build_obj_flags) $(includes) -o $@ $<

clean    :
	rm -rf $(object_dir) $(output_exec)

rebuild  : clean build
