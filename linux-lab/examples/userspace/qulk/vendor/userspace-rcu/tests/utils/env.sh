#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.
#

### Base paths to test suite ###
if [ "x${URCU_TESTS_SRCDIR:-}" = "x" ]; then
	URCU_TESTS_SRCDIR="/home/qwert/work/qulk/userspace/userspace-rcu/tests"
fi
export URCU_TESTS_SRCDIR

if [ "x${URCU_TESTS_BUILDDIR:-}" = "x" ]; then
	URCU_TESTS_BUILDDIR="/home/qwert/work/qulk/userspace/userspace-rcu/tests"
fi
export URCU_TESTS_BUILDDIR


### External Tools ###
if [ "x${URCU_TESTS_NPROC_BIN:-}" = "x" ]; then
	URCU_TESTS_NPROC_BIN="nproc"
fi
export URCU_TESTS_NPROC_BIN

if [ "x${URCU_TESTS_GETCONF_BIN:-}" = "x" ]; then
	URCU_TESTS_GETCONF_BIN="getconf"
fi
export URCU_TESTS_GETCONF_BIN

if [ "x${URCU_TESTS_TIME_BIN:-}" = "x" ]; then
	URCU_TESTS_TIME_BIN="time"
fi
export URCU_TESTS_TIME_BIN

