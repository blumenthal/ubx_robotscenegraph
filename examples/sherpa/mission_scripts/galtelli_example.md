#Usage

1. Configure to use correct map via ENV variable:
```
  export SWM_OSM_MAP_FILE=examples/maps/osm/map_galtelli.osm
```

2. Start SWM via: 
```
  ./run_sherpa_world_model.sh
  start_all()
```

3. Load map (within SWM console)
```
  load_map()
``` 

4. Start mission script (in new terminal) in the ``examples/sherpa/mission_script`` subfolder:
```
  cd examples/sherpa/mission_script
  python galtelli_example_mission1.py
``` 
