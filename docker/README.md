## SHERPA World Model Dockerfile

This folder contains a **Dockerfile** to create a Docker container with a SHERPA world model. 
 

### Installation

1. Install [Docker](https://www.docker.com/).

2. Build an image from this Dockerfile: either by invoking it relative to this folder:

```
  docker build -t sherpa-wm .
``` 

   or by using the Dockerfile from the repository: 

```
  docker build -t sherpa-wm https://raw.githubusercontent.com/blumenthal/ubx_robotscenegraph/master/docker/Dockerfile
```

### Usage

```
  docker run -it --net=host -p 8888:8888 -p 11411:11411 -p 12912:12912 sherpa-wm
  cd ./ubx_robotscenegraph
  ./run_sherpa_world_model.sh --no-ros
  start_all()
```

The UBX web interface is available on port 8888, so it can be opened by any browser
by typing: `http://localhost:8888/`.

You can kill the program with `ctrl+c` and then exit the docker container by typging `exit`.

### Troubleshooting
If you get the following error
```
  Post http:///var/run/docker.sock/v1.19/containers/create: dial unix /var/run/docker.sock: permission denied. Are you trying to connect to a TLS-enabled daemon without TLS?
```
from the docker commands, run them with `sudo`.

