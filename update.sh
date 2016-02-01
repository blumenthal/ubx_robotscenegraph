#!/bin/bash
#
# This script updates and recompiles the core C++ modules: 
# * ubx_robotscenegraph
# * brics_3d_function_block
# * brics_3d 
#
# Usage
# ------
# 
# ./update.sh 
#
# OR
# ./update.sh --no-sudo
#
# In case the system has no sudo command available. 
#
# OR
# ./update.sh --no-git
#
# In case no git pull sould be invoked. 
#
# Authors
# -------
#  * Sebastian Blumenthal (blumenthal@locomotec.com)
#

### Parameters ###

# Toggle this if sudo is not available on the system (e.g. a docker image).
SUDO="sudo"
if [ "$1" = "--no-sudo" ] || [ "$2" = "--no-sudo" ]; then
        SUDO=""
fi

USE_GIT="TRUE"
if [ "$1" = "--no-git" ] || [ "$2" = "--no-git" ]; then
        USE_GIT="FALSE"
fi


### Update & recompile ###

UBX_RSG_DIR="`pwd`"

if [ -z $BRICS_3D_DIR ]; then
        echo "Your BRICS_3D_DIR environment variable is not set. Please add it and rerun this script."
	exit
fi

if [ -z $FBX_MODULES ]; then
	echo "Your FBX_MODULES environment variable is not set. Please add it and rerun this script."
	exit
fi

echo "BRICS_3D_DIR: ${BRICS_3D_DIR}"
echo "FBX_DIR: ${FBX_MODULES}"
echo "UBX_RSG_DIR: ${UBX_RSG_DIR}"

cd ${BRICS_3D_DIR}/build
if [ "$USE_GIT" = "TRUE" ]; then
  echo "Pulling BRICS_3D from repository:"
  git pull origin master  
  #to be on the safe side: recall cmake
  cmake -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DUSE_HDF5=true -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true .. 
fi
echo "Recompiling BRICS_3D:"  
make -j4


echo "Recompiling FBX"
cd ${FBX_MODULES}/build
if [ "$USE_GIT" = "TRUE" ]; then
  echo "Pulling FBX from repository:"
  git pull origin master  
  cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DUSE_FBX=ON .. 
fi
make clean 
${SUDO} make install

echo "Recompiling UBX_RSG"
cd ${UBX_RSG_DIR}/build
if [ "$USE_GIT" = "TRUE" ]; then
  echo "Pulling UBX_RSG from repository:"
  git pull origin master  
  cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true .. 
fi
make clean 
${SUDO} make install

cd ..
echo "Done."

