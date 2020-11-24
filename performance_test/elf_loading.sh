#!/bin/bash
start="$(date -u +%s.%N)"
./trial main-1000.o
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "1000 cycle took $duration seconds"
echo "1000 cycle took $duration seconds" >> elf_loader_output

start="$(date -u +%s.%N)"
./trial main-10000.o
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "10000 cycle took $duration seconds"
echo "10000 cycle took $duration seconds" >> elf_loader_output

start="$(date -u +%s.%N)"
./trial main-100000.o
end="$(date -u +%s.%N)"
duration="$(bc <<<"$end-$start")"
echo "100000 cycle took $duration seconds"
echo "100000 cycle took $duration seconds" >> elf_loader_output
