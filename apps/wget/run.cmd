alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh wget.0.0.preopt "-lz" 2>&1 | tee $(curr_date)_out
#echo "Running wget"
#./wget.0.0.preopt.exe google.com
