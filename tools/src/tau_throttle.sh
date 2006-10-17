#!/bin/sh

if [ $# = 0 ] 
then
  echo "This tool generates a selective instrumentation file (called throttle.tau)"
  echo "from a program output that has TAU<id>: Throttle: Disabling ... messages."
  echo "The throttle.tau file can be sent to re-instrument a program using "
  echo "-optTauSelectFile=throttle.tau as an option to tau_compiler.sh/tau_f90.sh, etc."
  echo "Usage: tau_throttle.sh <output_file(s)> "
  exit 1
fi

echo "BEGIN_EXCLUDE_LIST" > throttle.tau
cat $*  | grep Throttle | awk '{print $4;}' | sort | uniq >>throttle.tau 
echo "END_EXCLUDE_LIST" >> throttle.tau
