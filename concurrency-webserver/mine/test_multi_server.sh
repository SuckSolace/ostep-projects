#!/bin/bash

# Check if the number of arguments is correct
if [ $# -lt 1 ]; then
  echo "Usage: $0 [num]"
  exit 1
fi

num=$1

# Perform the loop for the specified number of times
for ((i=1; i<=num; i++))
do
  echo "Loop $i"
  ./wclient localhost 10000 "/spin.cgi?2" &
done

wait

exit 0

