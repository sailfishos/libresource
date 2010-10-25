#!/bin/sh
# Run this to generate all the initial makefiles, etc. 
AUTOCONF="autoconf"
AUTOHEADER="autoheader"
AUTOMAKE="automake"
ACLOCAL="aclocal"
LIBTOOLIZE="libtoolize"
ACLOCAL_INCLUDES="-I m4"

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

mkdir -p build-aux

echo "Running $ACLOCAL..."
$ACLOCAL $ACLOCAL_INCLUDES $ACLOCAL_FLAGS || exit 1

echo "Running $AUTOHEADER..."
$AUTOHEADER || exit 1

echo "Running $AUTOCONF..."
$AUTOCONF || exit 1

echo "Running $LIBTOOLIZE --automake..."
$LIBTOOLIZE --automake -c || exit 1

echo "Running $AUTOMAKE..."
$AUTOMAKE -a -c || exit 1

