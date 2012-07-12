#!/bin/sh

export LD_LIBRARY_PATH=`pwd`/lib
exec bin/nox2 --fullscreen $*
