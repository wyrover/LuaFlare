#!/bin/bash

port=8080
instances=25

#if [ $(whoami) != "root" ]
#then
#	echo "error: you must be root to run this"
#	exit
#fi

./luaserver.lua --port=$port --threads=$instances