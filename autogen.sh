#!/bin/sh

# Use v1.11 for backwards compat with Ubuntu 12.04 LTS
export ACLOCAL=aclocal-1.11
export AUTOMAKE=automake-1.11

autoreconf -W portability -visfm
