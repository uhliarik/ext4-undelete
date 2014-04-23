#!/bin/bash
POWER_FAILURE_SCRIPT="./power_failure.sh"
CONFIG="config.cfg"
TEST_ITERATIONS=20

function error {
    local text="$1"
    echo "$text"
    exit 1
}

function test_config_vars {
    if [[ -z "$dev" ]] ; then
        error "Variable dev does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ -z "$snapshot_dev" ]] ; then
        error "Variable snapshot_dev does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ -z "$snapshot_name" ]] ; then
        error "Variable snapshot_name does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ -z "$origin_dev" ]] ; then
        error "Variable origin_dev does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ ! -b "$dev" ]] ; then
        error "Device $dev does NOT exist!"
    fi

    if [[ -z "$mount_point" ]] ; then
        error "Variable mount_point does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ ! -d "$mount_point" ]] ; then
        mkdir -p "$mount_point" || {
            error "Unable to create $mount_point directory!"
        }
    fi
}

# read config and test variables
source "$CONFIG" || {
    error "Unable to read config file: $CONFIG"
}
test_config_vars

# try to remove snapshot first (otherwise, system will freeze)
"$POWER_FAILURE_SCRIPT" -d "$origin_dev" "$snapshot_dev" "$snapshot_name"

# try to make snapshot and then run fsck on this snapshot
for i in $(seq 1 $TEST_ITERATIONS)
do
    "$POWER_FAILURE_SCRIPT" -s "$origin_dev" "$snapshot_dev" "$snapshot_name" > /dev/null 2>&1
    output=`fsck.ext4 -fn "/dev/mapper/$snapshot_name"`
    output_grep=`echo $output |grep Fix`
    if [ ! -z "$output_grep" ] ; then
	echo "Found problem: "
	echo "$output"
    fi
    "$POWER_FAILURE_SCRIPT" -d "$origin_dev" "$snapshot_dev" "$snapshot_name"
done

