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
# The RSG is used a mechanism to represent and exchange world model data.
#
# An instance of an RSG is embedded into a "World Model Agent". It is written
# in C++ and can be queries with its respective API. 
#
# A SHERPA World Model as deployed on a robot has on World Model Agent
# plus a set of communication componetns like ZMQ or ROS. The components
# are relaized with the Microbox (UBX) frame work that allows to represent
# the appliation in a system coposition model (.utc file). In a nutshell
# the following dependencies need t be installed:
# 
# * the BRICS_3D library with all its dependencis including
#   Boost, eigen3, HDF5, Libvariant for JSON, graphviz for debugging (optional)
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

echo "" 
echo "### Generic system dependencies for compiler, revision control, etc. ###"  
# Thie fist one is commented out because is install on most system already. 
#sudo apt-get install  \
#        git \
#	 mercurial \
#        cmake \
#        build-essential \


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
# one allows to use HDF5 in kind of messaging mode as we do. Further more the CMake
# scripts of BRICS_3D are serching for shared libraries so we have to active it in the 
# build process.  
make
make install
cd ..
cd ..

echo "Libvariant (JSON)"
hg clone https://bitbucket.org/gallen/libvariant
cd libvariant
mkdir build
cd build
cmake ..
make
make install
cd ..
cd ..

echo ""
echo "### Compile and install BRICS_3D ###"
git clone https://github.com/brics/brics_3d.git
cd brics_3d
mkdir build && cd build
cmake -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DUSE_HDF5=true -DUSE_JSON=true ..
make
cd ..
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
make
sudo make install
sudo ldconfig
cd ..

echo "## Compile and install UBX. ###"
git clone https://github.com/UbxTeam/microblx.git
cd microblx
source env.sh 
# This sourcing of env.sh is important and is often overlooked during manual compilation.
# It does configure a lot of LUA variables to make the UBX system models work.
make
cd ..




echo "Done."


