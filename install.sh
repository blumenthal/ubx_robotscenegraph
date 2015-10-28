#!/bin/bash
#
# This script (a) documents and (b) installs the SHERPA World Model software components.
# 
# Usage
# ------
#
# Simply call this sript wihtout any parameters: 
#	./install.sh
# OR
#	./install.sh --no-sudo
# In case the system has no sudo command available.
#
# If necessary make adjustments to the individual sections.
#
#
# Preface
# -------
# The core software library is the BRICS_3D perception and modeling library
# which contains an implementation of the "Robot Scene Graph (RSG)".
# The RSG is used as a mechanism to represent and exchange world model data.
#
# An instance of an RSG is embedded into a "World Model Agent". It is written
# in C++ and can be queries with its respective API. 
#
# A SHERPA World Model as deployed on a robot has one World Model Agent
# plus a set of communication componetns like ZMQ or ROS. The components
# are relalized with the Microblox (UBX) framework that allows to represent
# the appliation in a system coposition model (.utc file). 
# In a nutshell the following dependencies need to be satisfied:
# 
# * the BRICS_3D library with all its dependencies including
#   Boost, Eigen3, HDF5, Libvariant for JSON, graphviz for debugging (optional)
# * the UBX framework
# * the BRICS_3D integration into the UBX framework (brics_3d_function_blocks 
#   and ubx_robotscenegraph)
# * UBX modules for ZMQ including dependencies (CZMQ)
# * UBX modules for ROS (optional)  

# Toggle this if sudo is not available on the system.
SUDO="sudo"
if [ $1 = "--no-sudo" ]; then
        SUDO=""
fi
#SUDO=""

# Set parameter for prarallel builds
J="-j4"
#J=""

echo "" 
echo "### Generic system dependencies for compiler, revision control, etc. ###"  
# Thie fist one is commented out because is install on most system already. 
#sudo apt-get install  \
#        git \
#	 mercurial \
#        cmake \
#        build-essential \
#	 libtool \
#	 automake \
#	 libtool \
# 	 pkg-config

####################### BRICS_3D ##########################
echo "" 
echo "### Dependencies for the BRICS_3D libraries ###"
echo "Boost:"
${SUDO} apt-get install libboost-dev \
        libboost-thread-dev 

echo "Eigen 3:"
${SUDO} apt-get install libeigen3-dev

echo "Lib Cppunit for unit tests (optional):"
${SUDO} apt-get install libcppunit-dev

echo "HDF5: "
# This one is alway a bit tricky since there are many compil time
# options avialable and version changes have API breaks. 
# So far tested with verisons 1.8.9(minimum), 1.8.12 and 1.8.13.
# Note the option HDF5_1_8_12_OR_HIGHER must be set if a verison
# 1.8.12 or higher is used due to API incompatibilities.
wget www.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8.13/src/hdf5-1.8.13.tar.gz
tar -xvf hdf5-1.8.13.tar.gz
cd hdf5-1.8.13/
#NOTE: on Ubutnu 12.04 the CMake versions do not match. You can fix this by altering the HDF5 CMake files: 
grep "2.8.11" -l -r . | xargs sed -i 's/cmake_minimum_required (VERSION 2.8.11)/cmake_minimum_required (VERSION 2.8.7)/g'
mkdir build
cd build
cmake -DHDF5_BUILD_CPP_LIB=true -DHDF5_BUILD_HL_LIB=true -DBUILD_SHARED_LIBS=true ..
# A note on the used flags: the C++ API and the High Level (HL) APIs are used. The latter
# one allows to use HDF5 in a kind of messaging mode as we do. Furthermore, the CMake
# scripts of BRICS_3D are searching for shared libraries so we have to activate it in the 
# build process.  
make ${J}
make install
cd ..
cd ..

echo "Libvariant (JSON)"
hg clone https://bitbucket.org/gallen/libvariant
cd libvariant
mkdir build
cd build
cmake ..
cmake -DBUILD_SHARED_LIBS=true ..
# Note that UBX modules need the -fPIC flag, thus we have to enable the shared flag for Libvariant.
make ${J}
make install
cd ..
cd ..

echo ""
echo "### Compile and install BRICS_3D ###"
git clone https://github.com/brics/brics_3d.git
cd brics_3d
mkdir build && cd build
cmake -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DUSE_HDF5=true -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true ..
# A note on the used flags: For some reason the CMake find script has problems to find Eigen3 thus we currently
# have to provide the path as a flag. HDF5 and JSON support has to be enabled because it is not a default option.
# Furthermore, as we are using HDF5 1.8.13 we have to enable the HDF5_1_8_12_OR_HIGHER flag to account for 
# major changes in the HDF5 API.
make ${J}
cd ..
echo "export BRICS_3D_DIR=$PWD" >> ~/.bashrc 
#The BRICS_3D_DIR environment variable is needed for the other (below) modules to find BRICS_3D properly.
source ~/.bashrc .
cd ..

####################### Microblox #########################
echo ""
echo "### Dependencies for Microblox (UBX) framework. ###"

echo "Clang compiler:"
${SUDO} apt-get install clang

echo "Luajit 2.0.2:"
# NOTE Luajit needs to have at least verison 2.0.2  otherwise
# hard to trace errors will appear.
wget luajit.org/download/LuaJIT-2.0.2.tar.gz
tar -xvf LuaJIT-2.0.2.tar.gz 
cd LuaJIT-2.0.2
make ${J}
${SUDO} make install
${SUDO} ln -s /usr/local/bin/luajit /usr/local/bin/lua
${SUDO} ldconfig
cd ..

echo ""
echo "### Compile and install UBX. ###"
git clone https://github.com/UbxTeam/microblx.git
cd microblx
source env.sh 
# This sourcing of env.sh is important and is often overlooked during manual compilation.
# It does configure a lot of LUA variables to make the UBX system models work.
make
echo "export UBX_ROOT=$PWD" >> ~/.bashrc
# The UBX_ROOT environment variable is needed for the other (below) modules to find UBX properly.
echo "export UBX_MODULES=/usr/local/ubx" >> ~/.bashrc
# The UBX_MODULES variable points to the place where the UBX types, blocks etc. are installed. 
# It is used to load the types and modules at run-time.  we set it to the UBX default folder.
source ~/.bashrc .
cd ..

################ Communication modules #########################
echo ""
echo "### ZMQ communication modules  ###"

echo "ZMQ library:"

echo "CZMQ dependencies:"
git clone git://github.com/jedisct1/libsodium.git
cd libsodium
./autogen.sh
./configure 
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd ..

git clone git://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh
./configure
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd ..

echo "CZMQ library:"
git clone git://github.com/zeromq/czmq
cd czmq
./autogen.sh
./configure
make ${J}
${SUDO} make install
${SUDO} ldconfig

echo "CZMQ-UBX bridge"
git clone https://github.com/blumenthal/ubx
cd ubx/czmq_bridge
mkdir build
cd build
cmake ..
make ${J}
${SODO} make install
cd ..
cd ..
cd ..

## ROS (optional)

######### BRICS_3D integration into the UBX framework ###########
echo ""
echo "### BRICS_3D integration into the UBX framework  ###"

echo "brics_3d_function_blocks:"
git clone https://github.com/blumenthal/brics_3d_function_blocks.git
cd brics_3d_function_blocks
mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
make ${J}
${SODO} make install
cd ..
echo "export FBX_MODULES=$PWD" >> ~/.bashrc
#The FBX_MODULES environment variable is needed for the other (below) modules to find the BRICS_3D function blocks and typs.
source ~/.bashrc .
cd ..

echo "ubx_robotscenegraph"
# Note we do not need neccisarily need to check it out because this script is (currently) contained
# in ubx_robotscenegraph. Though for the sake of completness we repeat it here.
git clone https://github.com/blumenthal/ubx_robotscenegraph
cd ubx_robotscenegraph
mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true ..
make ${J}
${SODO} make install
cd ..
cd ..



echo "Done."


