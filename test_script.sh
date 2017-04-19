#!/usr/bin/env bash
read -p "Enter number of runs: " runs
runs=${runs:-1} # deafult 1

read -p "Enter destination file (default: 'tests/results.csv'): " dest
dest=${dest:-'tests/results.csv'}

> $dest # Clean out destination
echo "run_num,txn_count,aborts,run_time" >> $dest
for i in $(seq 1 $runs)
do
    results=$(./rundb)
    txn_count=$(echo $results | grep -P -o "(?<=txn_cnt=)\d+")
    abort_cnt=$(echo $results | grep -P -o "(?<=abort_cnt=)\d+")
    run_time=$(echo $results | grep -P -o "(?<=run_time=)\d+\.\d+")
    echo "$i,$txn_count,$abort_cnt,$run_time" >> $dest
    echo "Run $i done."
done

echo "Done! \nStatistics saved to: $dest"