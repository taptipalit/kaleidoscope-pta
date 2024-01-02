alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh lighttpd.0.0.preopt "-lz -lcrypt" 2>&1 | tee $(curr_date)_out
./lighttpd.0.0.preopt.exe -D -f lighty.conf
#wget http://localhost:8080/index.html
