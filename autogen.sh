#!/bin/bash

libtoolize
aclocal
automake --add-missing
autoconf
