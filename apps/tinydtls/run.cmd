alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh dtls-server.0.0.preopt 2>&1 | tee $(curr_date)_out
