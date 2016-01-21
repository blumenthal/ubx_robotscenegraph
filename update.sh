#!/bin/bash

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

echo "Recompiling BRICS_3D:"  
cd ${BRICS_3D_DIR}/build
make -j4


echo "Recompiling FBX"
cd ${FBX_MODULES}/build
make clean && make install

echo "Recompiling UBX_RSG"
cd ${UBX_RSG_DIR}/build
make clean && make install

cd ..
echo "Done."

