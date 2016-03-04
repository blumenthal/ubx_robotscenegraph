#!/bin/bash
#
# This script (a) documents the install procces and (b) installs the SHERPA World Model 
# software components if invoked as stated in the "Usage" section.
#
# 
# Usage
# ------
#
# Call this script wihtout any parameters:
# 
#	source ./install.sh -i
# 
# OR
#	source ./install.sh --no-sudo
#
# In case the system has no sudo command available. 
# 
# OR
# source ./install.sh --no-ros
# 
# In case the system has no ROS (Hydro) installtion. 
#
# The "source" command is important here because it adds environment variables
# that are required for the subsequent installation steps.
#
#
#
# Preface
# -------
# The core software library is the BRICS_3D perception and modeling library
# which contains an implementation of the "Robot Scene Graph (RSG)".
# The RSG is used as a mechanism to represent and exchange world model data.
#
# An instance of an RSG is embedded into a "World Model Agent". It is written
# in C++ and can be queried with its respective API. 
#
# A SHERPA World Model as deployed on a robot has one World Model Agent
# plus a set of communication componetns like ZMQ or ROS. The components
# are realized with the Microblox (UBX) framework that allows to represent
# the appliation in a system coposition model (.utc file). 
#
# In a nutshell the following dependencies need to be satisfied:
# 
# * the BRICS_3D library with all its dependencies including
#   Boost, Eigen3, HDF5, Libvariant for JSON, graphviz for debugging (optional)
# * the UBX framework
# * the BRICS_3D integration into the UBX framework (brics_3d_function_blocks 
#   and ubx_robotscenegraph)
# * UBX modules for ZMQ including dependencies (CZMQ)
# * UBX modules for ROS (optional)  
#
#
#
# Environment Variables
# ---------------------
#
# Please make sure the following environment variables are set. 
# The variables are also discussed in the individual installation step sections.
# 
# Variables for Microblox:
#
#
# | Name          | Description |
# | ------------- | ----------- |
# | UBX_ROOT      | Points to the installation folder of UBX. Used within the CMake scripts to discover the UBX library. |
# | UBX_MODULES   | Points to the the place where the UBX types, blocks etc. are installed. Used to load the types and modules at run-time |
# 
# Dependencies to BRICS_3D and HDF5:
# 
# 
# | Name          | Description |
# | ------------- | ----------- |
# | BRICS_3D_DIR  | Points to the installation folder of BRICS_3D. Used within the CMake scripts to discover the BRICS_3D library. |
# | FBX_MODULES   | Points to the the root folder of the BRICS_3D function blocks. Used to discover the rsg lua scripts.  |
# | HDF5_ROOT     | Points to the installation folder of HDF5. Use it in case it is not into installed to the default folders (/usr/local). |
# 
# example: 
#    export HDF5_ROOT=/opt/hdf5-1.8.13 
#
#
# Compatibility
# -------------
# This script has been succesfully tested on the following systems:
#
# * Ubuntu 12.04
# * Ubuntu 14.04
#
#
# Authors
# -------
#  * Sebastian Blumenthal (blumenthal@locomotec.com)
#  * Nico Huebel (nico.huebel@kuleuven.be)
#

show_usage() {
cat << EOF
Usage: ./${0##*/} -i [--no-sudo] [--no-ros] [--workspace-path=PATH] [--install-path=PATH] [-h|--help] [-j=VALUE] 
E.g. : ./${0##*/} -i --workspace-path=../ 

    -i                     Mandatory! Perform actual installation.
    -h|--help              Display this help and exit
    --no-sudo              In case the system has no sudo command available. 
    --no-ros               In case the system has no ROS (Hydro/Indigo) installation.
    --workspace-path=PATH  Path to where libraries and bulild. Default is ../
    --install-path=PATH    Path to where libraries and modeles are installed (make install) into.
                           (except for brics_3d). Default is /usr/local 
    -j=VALUE               used for make -jVAULE 

EOF
}

# Error handling
exec 3>&1 4>&2
trap 'exec 2>&4 1>&3; echo "ERROR occured. See install_err.log for details."; cd ${SCRIPT_DIR}; return 1' ERR SIGHUP SIGINT SIGQUIT SIGILL SIGABRT SIGTERM
exec > >(tee -a install.log) 2> >(tee -a install_err.log >&2)
#exec 1> install.log
#exec 2> install_err.log


rm -f install.log install_err.log

set -e
# Any subsequent(*) commands which fail will cause the shell script to exit immediately

# Default values
SUDO="sudo"
USE_ROS="TRUE"
WORKSPACE_DIR="../" # one above ubx_robotscenegraph to avoid a nested installation
INSTALL_DIR="/usr/local/"
J="-j1"
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# There should be at least one parameter.
if (( $# < 1 )); then
    show_usage
    exit 1
fi

# Handle command line options
while :; do
  case $1 in
    -i)   
      echo "Installing software."
      ;;
    -h|-\?|--help)   # Call a "show_help" function to display a synopsis, then exit.
      show_usage
      exit
      ;;
    --no-sudo)       
      SUDO=""
      ;;
    --no-ros)       
      USE_ROS="FALSE"
      ;;
    --workspace-path)       # Takes an option argument, ensuring it has been specified.
      if [ -n "$2" ]; then
           WORKSPACE_DIR=$2
          shift
       else
           printf 'ERROR: "--workspace-path" requires a non-empty option argument.\n' >&2
           exit 1
       fi
       ;;
    --workspace-path=?*)
       WORKSPACE_DIR=${1#*=} # Delete everything up to "=" and assign the remainder.
       ;;
    --workspace-path=)         # Handle the case of an empty --file=
      printf 'ERROR: "--workspace-path" requires a non-empty option argument.\n' >&2
      exit 1
      ;;

    --install-path)       # Takes an option argument, ensuring it has been specified.
      if [ -n "$2" ]; then
           INSTALL_DIR=$2
          shift
       else
           printf 'ERROR: "--install-path" requires a non-empty option argument.\n' >&2
           exit 1
       fi
       ;;
    --install-path=?*)
       INSTALL_DIR=${1#*=} # Delete everything up to "=" and assign the remainder.
       ;;
    --install-path=)         # Handle the case of an empty --file=
      printf 'ERROR: "--install-path" requires a non-empty option argument.\n' >&2
      exit 1
      ;;

    -j)       # Takes an option argument, ensuring it has been specified.
      if [ -n "$2" ]; then
           J="-j${2}"
          shift
       else
           printf 'ERROR: "-j" requires a non-empty option argument.\n' >&2
           exit 1
       fi
       ;;
    -j=?*)
       J_TMP=${1#*=} # Delete everything up to "=" and assign the remainder.
       J="-j${J_TMP}"
       ;;
    -j=)         # Handle the case of an empty --file=
      printf 'ERROR: "-j" requires a non-empty option argument.\n' >&2
      exit 1
      ;;


    --)              # End of all options.
       shift
       break
       ;;
    -?*)
       printf 'WARN: Unknown option (ignored): %s\n' "$1" >&2
        ;;
    *)               # Default case: If no more options then break out of the loop.
        break
  esac

  shift
done

echo "[Parameter] Sudo command is set to: ${SUDO}"
echo "[Parameter] USE_ROS is set to: ${USE_ROS}"
echo "[Parameter] WORKSPACE_DIR is set to: ${WORKSPACE_DIR}"
echo "[Parameter] INSTALL_DIR is set to: ${INSTALL_DIR}"
echo "[Parameter] Parallel build parameter for make is set to: ${J}"

#exit

# Go to workspace
cd ${WORKSPACE_DIR}
 

echo "" 
echo "### Generic system dependencies for compiler, revision control, etc. ###"  
# Thie fist one is commented out because is install on most system already. 
#sudo apt-get install  \
#        git \
#        mercurial \
#        cmake \
#        build-essential \
#	       libtool \
#	       automake \
#	       libtool \
# 	     pkg-config

####################### BRICS_3D ##########################
echo "" 
echo "### Dependencies for the BRICS_3D libraries ###"
echo "Boost:"
${SUDO} apt-get install -y libboost-dev \
        libboost-thread-dev \
        libboost-regex-dev

echo "Eigen 3:"
${SUDO} apt-get install -y libeigen3-dev

echo "Lib Cppunit for unit tests (optional):"
${SUDO} apt-get install -y libcppunit-dev

echo "Lib Xerces for loading Open Street Maps"
${SUDO} apt-get install -y libxerces-c-dev

echo "Lib yaml" #Reqired for installation on SHERPA Wasp (Odroid U3) 
${SUDO} apt-get install -y libyaml-dev

echo "HDF5: "
# This one is alway a bit tricky since there are many compile time
# options avialable and version changes have API breaks. 
# So far tested with verisons 1.8.9(minimum), 1.8.12 and 1.8.13.
# Note the option HDF5_1_8_12_OR_HIGHER must be set if a verison
# 1.8.12 or higher is used due to API incompatibilities.
if [ ! -d hdf5-1.8.13 ]; then
  wget www.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8.13/src/hdf5-1.8.13.tar.gz
  tar -xvf hdf5-1.8.13.tar.gz
fi
cd hdf5-1.8.13/
#NOTE: on Ubutnu 12.04 the CMake versions do not match. You can fix this by altering the HDF5 CMake files: 
grep "2.8.11" -l -r . | xargs sed -i 's/cmake_minimum_required (VERSION 2.8.11)/cmake_minimum_required (VERSION 2.8.7)/g'
mkdir build
cd build
cmake -DHDF5_BUILD_CPP_LIB=true -DHDF5_BUILD_HL_LIB=true -DBUILD_SHARED_LIBS=true -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
# A note on the used flags: the C++ API and the High Level (HL) APIs are used. The latter
# one allows to use HDF5 in a kind of messaging mode as we do. Furthermore, the CMake
# scripts of BRICS_3D are searching for shared libraries so we have to activate it in the 
# build process.  
make ${J}
${SUDO} make install
#echo "export HDF5_ROOT=/usr/local" >> ~/.bashrc
HDF5_ROOT=${INSTALL_DIR} 
cd ..
cd ..

echo "Libvariant (JSON)"
if [ ! -d libvariant ]; then
  hg clone https://bitbucket.org/gallen/libvariant
fi
cd libvariant
mkdir build
cd build
cmake ..
cmake -DBUILD_SHARED_LIBS=true -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
# Note that UBX modules need the -fPIC flag, thus we have to enable the shared flag for Libvariant.
make ${J}
${SUDO} make install
cd ..
cd ..

echo ""
echo "### Compile and install BRICS_3D ###"
# In case the HDF5 library is alredy pre-installed
# in another location you can use the environment
# variables HDF5_ROOT to point to another intallation location
if [ ! -d brics_3d ]; then
  git clone https://github.com/brics/brics_3d.git
  cd brics_3d
else
  cd brics_3d
  git pull origin master
fi

mkdir build && cd build
cmake -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DUSE_HDF5=true -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
# A note on the used flags: For some reason the CMake find script has problems to find Eigen3 thus we currently
# have to provide the path as a flag. HDF5 and JSON support has to be enabled because it is not a default option.
# Furthermore, as we are using HDF5 1.8.13 we have to enable the HDF5_1_8_12_OR_HIGHER flag to account for 
# major changes in the HDF5 API.
make ${J}
cd ..
#echo "export BRICS_3D_DIR=$PWD" >> ~/.bashrc
BRICS_3D_DIR=$PWD 
#The BRICS_3D_DIR environment variable is needed for the other (below) modules to find BRICS_3D properly.
#source ~/.bashrc .
cd ..

####################### Microblox #########################
echo ""
echo "### Dependencies for Microblox (UBX) framework. ###"

echo "Clang compiler:"
${SUDO} apt-get install clang

echo "Luajit 2.0.2:"
# NOTE Luajit needs to have at least verison 2.0.2  otherwise
# hard to trace errors will appear.
if [ ! -d LuaJIT-2.0.2 ]; then
  wget luajit.org/download/LuaJIT-2.0.2.tar.gz
  tar -xvf LuaJIT-2.0.2.tar.gz
fi 
cd LuaJIT-2.0.2
make ${J}
${SUDO} make install
#${SUDO} ln -s /usr/local/bin/luajit /usr/local/bin/lua
${SUDO} ldconfig
cd ..

echo ""
echo "### Compile and install UBX. ###"
if [ ! -d microblx ]; then
  git clone https://github.com/UbxTeam/microblx.git
fi
cd microblx
source env.sh 
# This sourcing of env.sh is important and is often overlooked during manual compilation.
# It does configure a lot of LUA variables to make the UBX system models work.
make
#echo "export UBX_ROOT=$PWD" >> ~/.bashrc
UBX_ROOT=$PWD
# The UBX_ROOT environment variable is needed for the other (below) modules to find UBX properly.
#echo "export UBX_MODULES=${INSTALL_DIR}/lib/ubx" >> ~/.bashrc
UBX_MODULES="${INSTALL_DIR}/lib/ubx"
# The UBX_MODULES variable points to the place where the UBX types, blocks etc. are installed. 
# It is used to load the types and modules at run-time.  we set it to the UBX default folder.
source ~/.bashrc .
cd ..

################ Communication modules #########################
echo ""
echo "### ZMQ communication modules  ###"

echo "CZMQ dependencies:"
if [ ! -d libsodium ]; then
  git clone https://github.com/jedisct1/libsodium.git
fi
cd libsodium
./autogen.sh
./configure 
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd ..

echo "ZMQ library:"
# Wee need a stable verison of libzmq du to incompatibilities with Zyre. 
# Unfortunately there are no tags github so we have to use a tar ball.
#git clone https://github.com/zeromq/libzmq.git
#cd libzmq
if [ ! -d zeromq-4.1.2 ]; then
  wget http://download.zeromq.org/zeromq-4.1.2.tar.gz
  tar -xvf zeromq-4.1.2.tar.gz
fi 
cd zeromq-4.1.2
./autogen.sh
./configure --with-libsodium=no --prefix=${INSTALL_DIR}/zeromq-4.1.2
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd ..

echo "CZMQ library:"
#git clone https://github.com/zeromq/czmq
#cd czmq
if [ ! -d czmq-3.0.2 ]; then
  wget https://github.com/zeromq/czmq/archive/v3.0.2.tar.gz
  tar zxvf v3.0.2.tar.gz
fi
cd czmq-3.0.2/
./autogen.sh
./configure --prefix=${INSTALL_DIR}/czmq-3.0.2
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd ..

echo "Zyre library:"
if [ ! -d zyre-1.1.0 ]; then
  wget https://github.com/zeromq/zyre/archive/v1.1.0.tar.gz
  tar zxvf v1.1.0.tar.gz
fi
cd zyre-1.1.0/
sh ./autogen.sh
./configure --prefix=${INSTALL_DIR}/zyre-1.1.0
make ${J}
${SUDO} make install
${SUDO} ldconfig
cd .

echo "CZMQ-UBX bridge"
# In case ZMQ or CZMQ libraries are alredy pre-installed
# in another location you can use the environment
# variables ZMQ_ROOT or CZMQ_ROOT to point to other
# intallation locations
if [ ! -d ubx ]; then 
  git clone https://github.com/blumenthal/ubx
  cd ubx/czmq_bridge
else
  cd ubx/czmq_bridge
  git pull origin master
fi
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
make ${J}
# Per default the UBX modules are installed to /usr/local
# this can be adjusted be setting the CMake variable
# CMAKE_INSTALL_PREFIX to another folder. Of course the
# the UBX_MODULES environment variable has to be adopted 
# to this new prefix: <my_install_prefix_path>/ubx/lib.
# Please install all UBX modules into one folder.
${SUDO} make install
cd ..
cd ..
cd ..


####################### ROS (optional) #######################
if [ "$USE_ROS" = "TRUE" ]; then
  echo "### ROS (Hydro) communication modules  ###"  

  #echo "ROS-Hydro:"
  #${SUDO} sh -c 'echo "deb http://packages.ros.org/ros/ubuntu precise main" > /etc/apt/sources.list.d/ros-latest.list' 
  #wget https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -O - | ${SUDO} apt-key add -
  #${SUDO} apt-get update
  #${SUDO} apt-get install ros-hydro-ros-base
  #echo "source /opt/ros/hydro/setup.bash" >> ~/.bashrc
  #source ~/.bashrc


  echo "UBX-ROS bridge:"
  if [ ! -d microblx_ros_bridge ]; then 
    git clone https://bitbucket.org/blumenthal/microblx_ros_bridge.git
    cd microblx_ros_bridge
  else
    cd microblx_ros_bridge
    git pull origin blumenthal
  fi
  git fetch && git checkout blumenthal
  mkdir build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
  make
  # Per default the UBX modules are installed to /usr/local
  # this can be adjusted be setting the CMake variable
  # CMAKE_INSTALL_PREFIX to another folder. Of course the
  # the UBX_MODULES environment variable has to be adopted 
  # to this new prefix: <my_install_prefix_path>/ubx/lib
  # Please install all UBX modules into one folder.
  ${SUDO} make install
  cd ..
  cd ..
fi

######### BRICS_3D integration into the UBX framework ###########
echo ""
echo "### BRICS_3D integration into the UBX framework  ###"

echo "brics_3d_function_blocks:"
# In case the BRICS_3D library is alredy installed
# in another location you can use the environment
# variable BRICS_3D_DIR to point to another installation location.
# The same applies to HDF5 (HDF5_ROOT) as well.
if [ ! -d brics_3d_function_blocks ]; then 
  git clone https://github.com/blumenthal/brics_3d_function_blocks.git
  cd brics_3d_function_blocks
else
  cd brics_3d_function_blocks
  git pull origin master
fi
mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DUSE_FBX=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
# In case you retrive a "Could NOT find BRICS_3D" delete the
# CMake cache and try again. Sometimes the FindBRICS_3D script 
# keeps the variables BRICS_3D_NOT_FOUND though the system is setup correctly. 
make ${J}
# Per default the UBX modules are installed to /usr/local
# this can be adjusted be setting the CMake variable
# CMAKE_INSTALL_PREFIX to another folder. Of course the
# the UBX_MODULES environment variable has to be adopted 
# to this new prefix: <my_install_prefix_path>/ubx/lib.
# Please install all UBX modules into one folder.
${SUDO} make install
cd ..
#echo "export FBX_MODULES=$PWD" >> ~/.bashrc
FBX_MODULES=$PWD
# The FBX_MODULES environment variable is needed for the other (below) 
# modules to find the BRICS_3D function blocks and typs.
#source ~/.bashrc .
cd ..

echo "ubx_robotscenegraph"
# Note we do not need necessarily need to check it out because this script is (currently) contained
# in ubx_robotscenegraph. Though, for the sake of completness we try to repeat it here.
#
if [ -d ubx_robotscenegraph ]; then
  echo "Folder ubx_robotscenegraph exists already. Pulling updates instead." 
  cd ubx_robotscenegraph
  git pull origin master  
else
  echo "Cloning a fresh copy of ubx_robotscenegraph."
  git clone https://github.com/blumenthal/ubx_robotscenegraph
  cd ubx_robotscenegraph
fi
mkdir build
cd build
# In case the BRICS_3D library is alredy installed
# in another location you can use the environment
# variable BRICS_3D_DIR to point to another installation location.
# The same applies to HDF5 (HDF5_ROOT) as well. 
cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..
# Please note that we have to apply the same compile time flags 
# and environemnt variables that have been used to compile the brics_3d library.
# In case you retrive a "Could NOT find BRICS_3D" delete the
# CMake cache and try again. Sometimes the FindBRICS_3D script 
# keeps the variables BRICS_3D_NOT_FOUND though the system is setup correctly. 
make ${J}
# Per default the UBX modules are installed to /usr/local
# this can be adjusted be setting the CMake variable
# CMAKE_INSTALL_PREFIX to another folder. Of course the
# the UBX_MODULES environment variable has to be adopted 
# to this new prefix: <my_install_prefix_path>/ubx/lib.
# Please install all UBX modules into one folder.
${SUDO} make install
cd ..
cd ..

cd ${SCRIPT_DIR} # go back

echo ""
echo "Done."
echo ""
echo ""
echo ""
echo ""
echo "############################ATTENTION###############################"
echo " ATTENTION: Please add the followign environment variables:"
echo ""
echo "export HDF5_ROOT=${HDF5_ROOT} >> ~/.bashrc"
echo "export UBX_ROOT=${UBX_ROOT} >> ~/.bashrc"
echo "export UBX_MODULES=${UBX_MODULES} >> ~/.bashrc"
echo "export BRICS_3D_DIR=${PWD} >> ~/.bashrc"
echo "export FBX_MODULES=${FBX_MODULES} >> ~/.bashrc"
echo "source ~/.bashrc ."
echo ""
echo "####################################################################"
echo "In case the ROS communication modules are used also start roscore:"
echo "	cd ./ubx_robotscenegraph"
echo "	roscore& "
echo "  ./run_sherpa_world_model.sh"
echo ""
echo "Then enter the following command and hit enter:"
echo "start_all()"
echo "####################################################################"
echo ""


set +e
echo "SUCCESS"
