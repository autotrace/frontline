#!/bin/sh
# Run this to generate all the initial makefiles, etc.

(autofig --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autofig' installed to compile Frontline."
  echo "See HACKING in frontline soruce directory."
  echo "Abort..."
  exit 1
}
autofig frontline-config.af

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="frontline"

(test -f $srcdir/configure.in \
  && test -d $srcdir/frontline \
  && test -f $srcdir/frontline/frontline.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level frontline directory"
    exit 1
}

. gnome-autogen.sh


