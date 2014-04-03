#!/bin/bash
FILE="fragmented_file"
CONFIG="config.cfg"
FRAGMENTED_BIN="./fragmented/fragmented"
OUT_FILE="generated.out"
DATA_RANGE=255

# how many files are created and deleted
ITERATIONS=10000

# every file is removed, if you want to remove every second file, 
# set REMOVE_RATIO to value 2
REMOVE_RATIO=1

# maximum is 500000 blocks, each block is 4KiB
MAX_BLOCK_COUNT=500000

function is_mounted {
    local mount_point="$1"
    cat "/proc/mounts" | grep "$mount_point" > /dev/null
    if [[ $? -eq 0 ]] ; then
        return 1;
    fi

    return 0;
}

function error {
    local text="$1"
    echo "$text"
    exit 1
}

function test_config_vars {
    if [[ -z "$dev" ]] ; then
        error "Variable dev does NOT exist. Please specify it in $CONFIG file"
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


if ! $(is_mounted $mount_point) ; then
    # unmount device
    umount "$dev" || {
	error "Failed to unmount: $dev"
    }
fi

# make "fresh" filesystem on device
mkfs.ext4 "$dev"

# mount device
if $(is_mounted $mount_point) ; then
    mount "$dev" "$mount_point" || {
        error "Failed to mount $dev"
    }
fi

# delete out file
echo -e "FILE NUMBER\tDATA VALUE\tBLOCKS COUNT\tREMOVE" > "$OUT_FILE"

for i in $(seq 1 $ITERATIONS)
do
    number=$RANDOM
    let "number %= $DATA_RANGE"

    block_count=$RANDOM
    let "block_count %= $MAX_BLOCK_COUNT"
    let "block_count += 4"

    # create file
    $FRAGMENTED_BIN "$mount_point/$FILE.$i" -c "$block_count" -d "$number" || {
        error "Failed to create fragmented file!"
    }

    # 50 % chance to remove file
    remove=$RANDOM
    remove_txt="no"
    let "remove %= $REMOVE_RATIO"
    if [ "$remove" -eq "0" ] ; then
        # remove file
	rm "$mount_point/$FILE.$i" || {
		error "Failed to remove file \"$mount_point/$FILE\""
	}
	remove_txt="yes"
    fi

    echo -e "$FILE.$i\t$number\t$block_count\t$remove_txt" >> "$OUT_FILE"
done


# unmount device
umount "$dev" || {
    error "Failed to unmount: $dev"
}

exit 0