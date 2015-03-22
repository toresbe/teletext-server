#!/usr/bin/env bash

export BOOST_DIR=/home/toresbe/projects/boost_1_57_0

clang -Wno-constant-logical-operand -Xclang -fcolor-diagnostics -g -DBOOST_LOG_DYN_LINK \
    -lstdc++ -lboost_log -lpthread -lboost_filesystem -lboost_system -lboost_log \
    --std=c++11 -I $BOOST_DIR -L $BOOST_DIR/stage/lib -rpath $BOOST_DIR/stage/lib -o config config.cpp #ttx_server main.cpp

exit $?
