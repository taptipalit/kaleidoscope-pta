alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh tiffcp.0.0.preopt "-lz" 2>&1 | tee $(curr_date)_out
#./tiffcp.0.0.preopt.exe -c lzw a.tif b.tif result.tif
