#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 09/09/2026
#

#
# clean-build.sh
# Completely cleans the build, both bazel and ccache
#
bazel clean --expunge
ccache --clear

