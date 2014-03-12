#!/bin/bash
FILE="fragmented_file"
BLOCKS_COUNT=5
DATA="64"
CONFIG="config.cfg"
FRAGMENTED_BIN="./fragmented/fragmented"
INODE="0"
MD5SUM=""

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

# unset exported variables
unset INODE
unset MD5SUM

# read config and test variables
source "$CONFIG" || {
    error "Unable to read config file: $CONFIG"
}
test_config_vars

# mount device
if $(is_mounted $mount_point) ; then
    mount "$dev" "$mount_point" || {
        error "Failed to mount $dev"
    }
fi

# create file
$FRAGMENTED_BIN "$mount_point/$FILE" -c "$BLOCKS_COUNT" -d "$DATA" || {
    error "Failed to create fragmented file!"
}

sync

# print md5sum of created file
INODE="$(ls -Li $mount_point/$FILE |cut -f1 -d' ')"
MD5SUM="$(md5sum $mount_point/$FILE |cut -f1 -d' ')"

# remount device
umount "$dev" || {
    error "Failed to unmount: $dev"
}

mount "$dev" "$mount_point" || {
    error "Failed to mount $dev"
}

# remove file
rm "$mount_point/$FILE" || {
    error "Failed to remove file \"$mount_point/$FILE\""
}

# unmount device
umount "$dev" || {
    error "Failed to unmount: $dev"
}

echo "$INODE"
echo "$MD5SUM"


exit 0