#!/bin/bash
# Copyright (C) 2012-2014 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301, USA.

function setup {
    # FIXME add error checking...
    # create placeholder DM device for the Snapshot DM device, still must be reloaded
    dmsetup create --notable $SNAPSHOT_NAME

    # create placeholder DM device for origin's -real device
    dmsetup create --notable $ORIGIN_REAL_NAME
    ORIGIN_MAPPING=$(dmsetup table $ORIGIN_NAME)
    echo "$ORIGIN_MAPPING" | dmsetup --verifyudev load $ORIGIN_REAL_NAME
    dmsetup resume $ORIGIN_REAL_NAME

    if [ ! -b $ORIGIN_REAL_DM_DEV ]; then
	echo "Unable to create $ORIGIN_REAL_DM_DEV"
	exit 1
    fi

    # just load the origin mapping with the snapshot target, still must suspend and resume origin
    echo "0 $ORIGIN_DEV_SIZE snapshot-origin $ORIGIN_REAL_DM_DEV" | dmsetup load $ORIGIN_NAME

    # create the snapshot cow device
    dmsetup create --notable $SNAPSHOT_COW_NAME
    echo "0 $SNAPSHOT_DEV_SIZE linear $SNAPSHOT_DEV 0" | dmsetup load $SNAPSHOT_COW_NAME
    dmsetup suspend --nolockfs $SNAPSHOT_COW_NAME
    dmsetup resume $SNAPSHOT_COW_NAME
    # wipe the exception store header area
    dd if=/dev/zero of=$SNAPSHOT_COW_DM_DEV bs=512 count=$CHUNK_SIZE_SECTORS oflag=direct

    # reload the snapshot DM device, wait to suspend/resume
    echo "0 $ORIGIN_DEV_SIZE snapshot $ORIGIN_REAL_DM_DEV $SNAPSHOT_COW_DM_DEV P $CHUNK_SIZE_SECTORS" | dmsetup load $SNAPSHOT_NAME
    dmsetup resume $SNAPSHOT_NAME

    # suspend the origin devices
    dmsetup suspend --nolockfs $ORIGIN_NAME
    dmsetup suspend --nolockfs $ORIGIN_REAL_NAME

    # suspend the snapshot device
    dmsetup suspend --nolockfs $SNAPSHOT_NAME

    # resume devices
    dmsetup resume $ORIGIN_REAL_NAME
    dmsetup resume $SNAPSHOT_NAME
    dmsetup resume $ORIGIN_NAME
}

function teardown {
    dmsetup remove $SNAPSHOT_NAME

    ORIGIN_MAPPING=$(dmsetup table $ORIGIN_REAL_NAME)
    echo "$ORIGIN_MAPPING" | dmsetup load $ORIGIN_NAME
    dmsetup suspend $ORIGIN_NAME
    dmsetup resume $ORIGIN_NAME

    dmsetup remove $ORIGIN_REAL_NAME
    dmsetup remove $SNAPSHOT_COW_NAME
}

function usage {
    echo "Usage: $SCRIPTNAME [options] OriginDmName SnapshotDevice SnapshotName"
    echo "options:"
    echo "    -s|--setup  -- setup dirty snapshot"
    echo "    -d|--delete -- delete dirty snapshot"
    echo "    -c|--chunksize CHUNK_SIZE_SECTORS -- snapshot store's chunksize"
    echo
    echo "OriginDmName   - e.g.: dmsetup table <OriginDmName>"
    echo "SnapshotDevice - e.g.: /dev/sdb or /dev/mapper/vg-lv"
    echo "SnapshotName   - e.g.: snapshot"
    echo
    exit 1
}


CREATE=0
DELETE=0
CHUNK_SIZE_SECTORS=8

SCRIPTNAME=$(basename $0)

OPTIONS=`getopt -o sdc:xh \
    -l setup,delete,chunksize:,debug,help \
    -n "${SCRIPTNAME}" -- "$@"`
[ $? -ne 0 ] && usage
eval set -- "$OPTIONS"

while true
do
    case $1 in
	-s|--setup)
	    CREATE=1; shift
	    ;;
	-d|--delete)
	    DELETE=1; shift
	    ;;
	-c|--chunksize)
	    CHUNK_SIZE_SECTORS=$2
	    shift; shift
	    ;;
	-x|--debug)
	    shift
	    set -x
	    ;;
	-h|--help)
	    usage
	    ;;
	--)
	    shift; break
	    ;;
	*)
	    echo $OPTION
	    usage
	    ;;
    esac
done

if [ $CREATE -eq 0 -a $DELETE -eq 0 -o \
     $CREATE -eq 1 -a $DELETE -eq 1 ]; then
    echo "Must specify either -s or -d"
    usage
fi

ORIGIN_NAME=$1
SNAPSHOT_DEV=$2
SNAPSHOT_NAME=$3

if [ ! -b "$SNAPSHOT_DEV" ]; then
    echo "Snapshot cow device isn't a valid block device"
    usage
fi

ORIGIN_DM_DEV=/dev/mapper/$ORIGIN_NAME
SNAPSHOT_DM_DEV=/dev/mapper/$SNAPSHOT_NAME

ORIGIN_REAL_NAME=${ORIGIN_NAME}-real
ORIGIN_REAL_DM_DEV=/dev/mapper/$ORIGIN_REAL_NAME
SNAPSHOT_COW_NAME=${SNAPSHOT_NAME}-cow
SNAPSHOT_COW_DM_DEV=/dev/mapper/$SNAPSHOT_COW_NAME

ORIGIN_DEV_SIZE=$(blockdev --getsize $ORIGIN_DM_DEV)
SNAPSHOT_DEV_SIZE=$(blockdev --getsize $SNAPSHOT_DEV)

if [ $CREATE -eq 1 ]; then
    setup
elif [ $DELETE -eq 1 ]; then
    teardown
fi
