#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# This module declares a macro for debugging purposes.
# Simply place a call to set_break() in your build files and you will get a prompt

macro(set_break)
    include(cmakerepl)
endmacro()

# Mechanism for switching complex commands into and out of the foreground
# Ninja restricts access to stdio for running commands by default.
# Any output will only be printed once the task has completed.
# This can be confusing for commands that take a long time to complete or hang
# on failure instead of returning.
# Foregrounding the tasks by setting USES_TERMINAL solves this problem as Ninja
# lets the process directly access stdio. However this places the command into
# a job pool that only allows one command to run at a time.
# Therefore, CMakeForegroundComplexCommands is provided to allow bulk switching
# of these commands between foreground and background. The intention is that it
# is typically left off unless a build requires debugging in which case performance
# is no longer as important as being able to get more helpful debug info of the
# failing command.
# Note: This requires correctly annotating complex commands with USES_TERMINAL_DEBUG
# in the correct add_custom_command calls.
set(CMakeForegroundComplexCommands OFF CACHE BOOL "Set USES_TERMINAL on specially marked tasks. \
This makes the task run in the foreground and can directly access stdio. This is helpful for \
debugging tasks that take a long time, or want to insert a breakpoint into the process. \
Note: Only one task can run in foreground at a time.")
mark_as_advanced(CMakeForegroundComplexCommands)
if(CMakeForegroundComplexCommands)
    set(USES_TERMINAL_DEBUG USES_TERMINAL)
else()
    set(USES_TERMINAL_DEBUG "")
endif()
