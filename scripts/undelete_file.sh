#!/bin/bash
CONFIG="config.cfg"
UNDELETE_BIN="./../ext-undelete"
OUTPUT="/tmp/undelete.file"
INODE=""
MD5SUM=""
MD5SUM_UNDEL=""

function test_config_vars {
    if [[ -z "$dev" ]] ; then
        error "Variable dev does NOT exist. Please specify it in $CONFIG file"
    fi

    if [[ ! -b "$dev" ]] ; then
        error "Device $dev does NOT exist!"
    fi
}

function error {
    local text="$1"
    echo "$text"
    exit 1
}

# read config and test variables
source "$CONFIG" || {
    error "Unable to read config file: $CONFIG"
}
test_config_vars

# read first two lines: first is inode, second is md5sum
read INODE
read MD5SUM

echo "Undeleting inode no.: $INODE"
echo "MD5SUM of delete file is: $MD5SUM"

if [[ -z "$INODE" ]] ; then
    error "Variable INODE is not set!"
fi

if [[ -z "$MD5SUM" ]] ; then
    error "Variable MD5SUM is not set!"
fi

$UNDELETE_BIN $dev -i $INODE -o $OUTPUT > /dev/null || {
    error "Unable to undelete file!"
}

# print md5sum of created file
MD5SUM_UNDEL="$(md5sum $OUTPUT |cut -f1 -d' ')"

if [[ "$MD5SUM" != "$MD5SUM_UNDEL" ]] ; then
    error "MD5SUMS of deleted and undeleted file are different!"
else 
    echo "Undelete was successfull. MD5 sums are equal."
fi

exit 0
