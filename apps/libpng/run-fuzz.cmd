alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope_fuzz.sh pngcp.0.0.preopt "-lz" 2>&1 | tee $(curr_date)_out

rm -rf input_directory
mkdir input_directory
cp png-real.png input_directory
python create.py

# One run
./pngcp.0.0.preopt_gcov.exe png-real.png out.png # Run GCOV instrumented binary, this will generate default.profraw
llvm-profdata merge -o default.profdata default.profraw 
llvm-cov report ./pngcp.0.0.preopt_gcov.exe -instr-profile=./default.profdata

# Run AFL
#../../AFLplusplus/afl-fuzz -i input_directory -o output_directory -t 2000 ./pngcp.0.0.preopt_fuzz.exe
