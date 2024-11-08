#!/bin/bash

PROJECT_ROOT=$(pwd)

add_path()
{
    local temp_path
    temp_path=$(echo $PATH | tr ':' '\n' | grep $1)
    if [ -z $temp_path ]
    then
        export PATH=$1:$PATH
    fi
}

add_python_path()
{
    local temp_path
    temp_path=$(echo $PYTHONPATH | tr ':' '\n' | grep $1)
    if [ -z $temp_path ]
    then
        export PYTHONPATH=$1:$PYTHONPATH
    fi
}

add_path $PROJECT_ROOT/buildtools/prebuilts/linux/ninja
add_path $PROJECT_ROOT/buildtools/prebuilts/linux/gcc-arm-none-eabi-8-2019-q3-update/bin
add_path $PROJECT_ROOT/buildtools/prebuilts/linux/cmake/bin

add_python_path $PROJECT_ROOT/buildtools/tools/python3

python3 xybuild_ninja.py $* || exit -1
