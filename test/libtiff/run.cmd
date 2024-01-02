alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh tiffcrop-full "-lz" 2>&1 | tee $(curr_date)_out
./tiffcrop-full.exe -c lzw a.tif b.tif result.tif
