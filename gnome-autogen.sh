#!/bin/sh

if test -z "USE_GNOME2_MACROS" ; then
  USE_GNOME2_MACROS=0
fi

if test "$USE_GNOME2_MACROS" = 1 ; then
  if test -z "$GNOME2_DIR" ; then
    GNOME_COMMON_DATADIR="/usr/local/share"
  else
    GNOME_COMMON_DATADIR="$GNOME2_DIR/share"
  fi
  GNOME_COMMON_MACROS_DIR="$GNOME_COMMON_DATADIR/aclocal/gnome2-macros"
else
  if test -z "$GNOME_DIR" ; then
    GNOME_COMMON_DATADIR="/usr/local/share"
  else
    GNOME_COMMON_DATADIR="$GNOME_DIR/share"
  fi
  GNOME_COMMON_MACROS_DIR="$GNOME_COMMON_DATADIR/aclocal/gnome-macros"
fi

export GNOME_COMMON_DATADIR
export GNOME_COMMON_MACROS_DIR

ACLOCAL_FLAGS="-I $GNOME_COMMON_MACROS_DIR $ACLOCAL_FLAGS"
export ACLOCAL_FLAGS

. $GNOME_COMMON_MACROS_DIR/autogen.sh

