#!/usr/bin/env bash

HOSTNAME=`hostname`

function copy-spmln() {
  if [ -d "data" ]; then
    echo "data directory already exists"
    return
  fi
  mkdir -p ./data
  cp /opt/SPMcode/testdir/files/* ./data/
}

function copy-r7425renaissance() {
  if [ -d "data" ]; then
    echo "data directory already exists"
    return
  fi
  mkdir -p ./data
  cp /opt/SPMcode/A2/files/* ./data/
}

function unknown() {
  if [ -d "data" ]; then
    echo "data directory already exists"
    return
  fi
  mkdir -p ./data
  echo "Place test data in <data> directory"
}

if [ "$HOSTNAME" == "spmln" ]; then
  copy-spmln
elif [ "$HOSTNAME" == "r7425renaissance" ]; then
  copy-r7425renaissance
else
  unknown
fi