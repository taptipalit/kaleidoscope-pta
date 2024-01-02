alias curr_date='date "+%d_%m_%Y_%H_%M_%S"'
../run_kaleidoscope.sh ssl_server2.0.0.preopt 2>&1 | tee $(curr_date)_out
echo "Running ssl_server"
./ssl_server2.0.0.preopt.exe
#/home/tpalit/test-apps/mbedtls-2.16.5/programs/ssl/ssl_client2
