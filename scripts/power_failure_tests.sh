#!/bin/bash

# start creating and removing files
./create_rm_test.sh &

# create snapshot and test it with fsck
./test_snapshot.sh
