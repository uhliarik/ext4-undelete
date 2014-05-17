#!/bin/bash
TRACE_PATH="/sys/kernel/debug/tracing"

# nastaveni typu trasovace
echo "function_graph" > "$TRACE_PATH/current_tracer"

# zapsani PID tohoto procesu
echo "$$" > "$TRACE_PATH/set_ftrace_pid"

# zapnuti trasovani
echo "1"  > "$TRACE_PATH/tracing_on"

# parametr tohoto skriptu bude 
# spusten jako trasovany program
exec $*
