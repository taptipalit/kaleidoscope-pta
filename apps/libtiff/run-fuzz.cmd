alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope_fuzz.sh tiffcp.0.0.preopt_gcov  "-lz" 2>&1 | tee $(curr_date)_out
./tiffcp.0.0.preopt_gcov_gcov.exe -c lzw a.tif b.tif result.tif
llvm-profdata merge -o default.profdata default.profraw 
llvm-cov report ./tiffcp.0.0.preopt_gcov_gcov.exe  -instr-profile=./default.profdata
