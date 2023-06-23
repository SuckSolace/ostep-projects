#!/bin/bash

# Check if the number of arguments is correct
if [ $# -lt 1 ]; then
  echo "Usage: $0 [num]"
  exit 1
fi

num=$1

#Perform the loop for the specified number of times
for ((i=1; i<=num; i++))
do
  echo "Loop $i"
  ./wclient localhost 10000 "/csapp.h" & #6621
  ./wclient localhost 10000 "/ex.html" & #3054
  ./wclient localhost 10000 "/io_helper.c" &  #2536
  ./wclient localhost 10000 "/common_threads.h" & #1898
  ./wclient localhost 10000 "/common.h" &  #341
  ./wclient localhost 10000 "/common.h" &  #341
done

wait

exit 0

