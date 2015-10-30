## SHERPA World Model Dockerfile

This folder contains a **Dockerfile** to create a Docker container with a SHERPA world model. 
 

### Installation

1. Install [Docker](https://www.docker.com/).

2. Build an image from this Dockerfile: either by invoking it relative to this folder:

```
  docker build -t sherpa-wm .
``` 

   or by using the Dockerfile form the repository: 

```
  docker build -t sherpa-wm https://raw.githubusercontent.com/blumenthal/ubx_robotscenegraph/master/docker/Dockerfile
```

### Usage

```
  docker run -it -p 8888:8888 -p 11411:11411 -p 12911:12911 sherpa-wm
  cd ./ubx_robotscenegraph
  ./run_sherpa_world_model.sh --no-ros
  start_all()
```

The UBX web interface is available on port 8888, so it can be opened by any browser
by typing: `http://localhost:8888/`.
