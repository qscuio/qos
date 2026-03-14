# SPDX-License-Identifier: GPL-2.0-only
# builds the trace events example kernel modules;
# then to use one (as root):  insmod <module_name.ko>

# If you include a trace header outside of include/trace/events
# then the file that does the #define CREATE_TRACE_POINTS must
# have that tracer file in its main search path. This is because
# define_trace.h will include it, and must be able to find it from
# the include/trace directory.
#
# Here trace-events-sample.c does the CREATE_TRACE_POINTS.
#
CFLAGS_trace-events-sample.o := -I$(src)
CFLAGS_trace_custom_sched.o := -I$(src)

obj-m += trace-events-sample.o
obj-m += trace_custom_sched.o


