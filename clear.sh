#!/bin/sh
__DIR__=$(cd "$(dirname "$0")";pwd)

set -e
cd ${__DIR__}
set +e
find . -name \*.lo -o -name \*.o | xargs rm -f
find . -name \*.la -o -name \*.a | xargs rm -f
find . -name \*.so | xargs rm -f
find . -name .libs -a -type d|xargs rm -rf
rm -f modules/* libs/*
