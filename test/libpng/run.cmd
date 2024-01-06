alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh pngcp.0.0.preopt "-lz" 2>&1 | tee $(curr_date)_out
./pngcp.0.0.preopt.exe png-real.png out.png # Run the binary

