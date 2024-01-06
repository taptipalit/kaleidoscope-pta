alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope_fuzz.sh ssl_server2.0.0.preopt_withgcov 2>&1 | tee $(curr_date)_out
echo "Running ssl_server. Run the client and then kill the server and execute profile.cmd"
./ssl_server2.0.0.preopt_withgcov_gcov.exe
# Command for child
#/home/tpalit/test-apps/mbedtls-2.16.5/programs/ssl/ssl_client2
