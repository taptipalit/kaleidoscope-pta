alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh xmllint-full "-lz" 2>&1 | tee $(curr_date)_out
#echo "Running xmlling"
#./xmllint-full.exe test.xml
