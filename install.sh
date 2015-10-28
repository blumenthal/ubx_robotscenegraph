#!/bin/bash
#
# This script (a) documents and (b) installs the SHERPA World Model software components.
# 
# Usage
# ------
#
# Simply call this sript wihtout any parameters: 
#	./install.sh
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


echo "### Generic system dependencies for compiler, revision control, etc. ###"  
# Thie fist one is commented out because is install on most system already. 
#sudo apt-get install  \
#        git \
#        cmake \
#        build-essential \


####################### BRICS_3D ##########################
echo "### Dependencies for the BRICS_3D libraries ###"
echo "Boost:"
sudo apt-get libboost-dev \
        libboost-thread-dev 

echo "Eigen 3:"
sudo apt-get install libeigen3-dev

echo "Lib Cppunit for unit tests (optional):"
sudo apt-get install libcppunit-dev

echo "HDF5: "
# This one is alway a bit tricky since there are many compil time
# options avialable and version changes have API breaks.

echo "Libvariant (JSON)"

echo "### Compile and install BRICS_3D ###"
git clone https://github.com/brics/brics_3d.git
cd brics_3d
mkdir build && cd build
cmake -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DUSE_HDF5=true -DUSE_JSON=true ..
make
cd ..
cd ..

####################### Microblox #########################
echo "### Dependencies for Microblox (UBX) framework. ###"

echo "Clang compiler:"
sudo apt-get install clang

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


