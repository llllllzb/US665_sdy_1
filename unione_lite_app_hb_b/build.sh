#!/bin/bash
ulimit -n 8192
if [ "$1" == clean ]; then
  make clean
  echo "make clean finished"
elif [ "$1" == all ]; then
  make clean
  make -j 4
  echo "clean&make finished"
  mkdir -p output
  cp cpu/br23/tools/jl_isd.fw output/uni_app.fw
elif [ "$1" == res ]; then
  cd ./apps/soundbox/smartbox/unisound/unires/scripts && python2 res_build_tool.py && cd -
  echo "build res finished"
else
  make -j 4
  echo "make finished"
  mkdir -p output
  cp cpu/br23/tools/jl_isd.fw output/uni_app.fw
fi
