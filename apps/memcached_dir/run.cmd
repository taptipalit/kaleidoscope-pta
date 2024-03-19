alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh memcached-libevent -pthread 2>&1 | tee $(curr_date)_out
echo "Running memcached"
#./memcached-libevent.exe -t 1
