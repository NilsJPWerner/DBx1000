echo "setup,thread_cnt,txn_count,aborts,run_time" >>  tests/results.csv

cp config.h temp
cp tests/config_plain_silo.h config.h

make -j
for i in {0..5}; do
    results="$(./rundb)"
    thread_cnt=$(echo $results | grep -E -o 'Thread Count: [0-9]{1,4}' | sed 's/Thread Count: //')
    txn_count=$(echo $results | grep -E -o '\[summary\] txn_cnt=[0-9]{3,6}' | sed 's/\[summary\] txn_cnt=//')
    aborts=$(echo $results | grep -E -o 'abort_cnt=[0-9]{1,6}' | sed 's/abort_cnt=//')
    run_time=$(echo $results | grep -E -o 'run_time=[0-9.]{3,10}' | sed 's/run_time=//')
    echo "silo,$thread_cnt,$txn_count,$aborts,$run_time" >> tests/results.csv
done

rm config.h
cp tests/config_plain_silo_atomic.h config.h

make -j
for i in {0..5}; do
    results="$(./rundb)"
    thread_cnt=$(echo $results | grep -E -o 'Thread Count: [0-9]{1,4}' | sed 's/Thread Count: //')
    txn_count=$(echo $results | grep -E -o '\[summary\] txn_cnt=[0-9]{3,6}' | sed 's/\[summary\] txn_cnt=//')
    aborts=$(echo $results | grep -E -o 'abort_cnt=[0-9]{1,6}' | sed 's/abort_cnt=//')
    run_time=$(echo $results | grep -E -o 'run_time=[0-9.]{3,10}' | sed 's/run_time=//')
    echo "silo_atomic,$thread_cnt,$txn_count,$aborts,$run_time" >> tests/results.csv
done

rm config.h
cp tests/config_record_temp.h config.h

make -j
for i in {0..5}; do
    results="$(./rundb)"
    thread_cnt=$(echo $results | grep -E -o 'Thread Count: [0-9]{1,4}' | sed 's/Thread Count: //')
    txn_count=$(echo $results | grep -E -o '\[summary\] txn_cnt=[0-9]{3,6}' | sed 's/\[summary\] txn_cnt=//')
    aborts=$(echo $results | grep -E -o 'abort_cnt=[0-9]{1,6}' | sed 's/abort_cnt=//')
    run_time=$(echo $results | grep -E -o 'run_time=[0-9.]{3,10}' | sed 's/run_time=//')
    echo "record_temp,$thread_cnt,$txn_count,$aborts,$run_time" >> tests/results.csv
done

rm config.h
mv temp config.h
