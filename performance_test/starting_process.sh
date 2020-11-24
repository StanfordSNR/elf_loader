#!/bin/bash
start="$(date -u +%s.%N)"
for i in {1..10000}; do ./main-1000; done
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "1000 cycle took $duration seconds"
echo "1000 cycle took $duration seconds" >> starting_process_output

start="$(date -u +%s.%N)"
for i in {1..10000}; do ./main-10000; done
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "10000 cycle took $duration seconds"
echo "10000 cycle took $duration seconds" >> starting_process_output

start="$(date -u +%s.%N)"
for i in {1..10000}; do ./main-100000; done
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "100000 cycle took $duration seconds"
echo "100000 cycle took $duration seconds" >> starting_process_output
