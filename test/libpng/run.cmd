alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh pngcp-full "-lz" 2>&1 | tee $(curr_date)_out
./pngcp-full.exe png-real.png out.png
