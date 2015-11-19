# This Dockerfile starts from Umbutn 12.04 and installs
# all software components for a SHERPA world model.
#
# In a nutshell this image is a basic Ubuntu with
# develoment tools (and a ROS hydro installation).
# The more spefific software compnents are installed
# with the "install.sh" script.
# 

FROM ubuntu:12.04
MAINTAINER Sebastian Blumenthal

ENV WORKSPACE_DIR /workspace
WORKDIR /workspace

RUN apt-get -y update && apt-get install -y \
	git \
	mercurial \
	cmake \
	build-essential \
	nano \
	wget \
	libboost-dev \ 
	libboost-thread-dev \
	libeigen3-dev \
	libcppunit-dev \
	clang \
	libtool \
	automake \
	pkg-config

# This replaces default command interpreter from sh to bash.
RUN ln -snf /bin/bash /bin/sh

## ROS Hydro (optional)
## cf. https://gist.github.com/tntmarket/8e1eae6ef39ae584e832
#
#RUN sh -c 'echo "deb http://packages.ros.org/ros/ubuntu precise main" > /etc/apt/sources.list.d/ros-latest.list' 
#RUN wget https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -O - | apt-key add -
#RUN apt-get update -y && apt-get install -y ros-hydro-ros-base
#RUN /bin/bash -c "source /opt/ros/hydro/setup.bash"
#RUN echo "source /opt/ros/hydro/setup.bash" >> ~/.bashrc

# Get the install script
RUN cd /workspace; git clone https://github.com/blumenthal/ubx_robotscenegraph.git

# The environment vaiables as set by the below install.sh script are not set properly,
# thus we set them beforehands.
ENV BRICS_3D_DIR /workspace/brics_3d
ENV UBX_ROOT /workspace/microblx
ENV UBX_MODULES /usr/local/lib/ubx
ENV FBX_MODULES /workspace/brics_3d_function_blocks
RUN cd /workspace; /bin/bash -c "source ~/.bashrc"; /bin/bash -c "source ./ubx_robotscenegraph/install.sh --no-sudo --no-ros"
# Use the one below for instllation of the ROS brisge as well:
#RUN cd /workspace; /bin/bash -c "source ~/.bashrc"; /bin/bash -c "source /opt/ros/hydro/setup.bash"; /bin/bash -c "source ./ubx_robotscenegraph/install.sh --no-sudo"

# Default ports for the web interface and ZMQ puplishers. 
# Port definitions depend on the used system model compuition (.usc) file
#
# * 8888  for UBX web interface
# * 11411 for RSG with HDF5 protocol encoding via ZMQ-PUB
# * 12912 for RSG with JSON protocol encoding via ZMQ-PUB
# * 22422 for RSG JSON queries via ZMQ REQ-REP
# 
EXPOSE 8888
EXPOSE 11411
EXPOSE 12912
EXPOSE 22422

CMD /bin/bash

