#!/bin/bash
# cd ~ && sudo umount /mnt/c
# sudo mount -t drvfs C:\\ /mnt/c -o metadata
echo "---Compiling tests---"
gcc say_recv_test.c -o say_recv_test
gcc many_clients_test.c -o many_clients_test 
gcc multiple_domains_test.c -o multiple_domains_test
gcc say_recvcont_test.c -o say_recvcont_test
gcc disconnect_test.c -o disconnect_test
gcc pingpong_test.c -o pingpong_test
gcc invalid_protocol_test.c -o invalid_protocol_test
gcc invalid_message_test.c -o invalid_message_test
gcc client_disconnect_say_test.c -o client_disconnect_say_test

echo "---Running tests---"
echo "Running client_disconnect_say_test"
./test & ./client_disconnect_say_test | diff - client_disconnect_say_test.out || echo "Test client_disconnect_say_test: failed!"
echo "Running invalid_message_test"
./test & ./invalid_message_test | diff - invalid_message_test.out || echo "Test invalid_message_test: failed!"
echo "Running invalid_protocol_test"
./test & ./invalid_protocol_test | diff - invalid_protocol_test.out || echo "Test invalid_protocol_test: failed!"
echo "Running many_clients_test"
./test & ./many_clients_test | diff - many_clients_test.out || echo "Test many_clients_test: failed!"
echo "Running say_recv_test"
./test & ./say_recv_test | diff - say_recv_test.out || echo "Test say_recv_test: failed!"
echo "Running multiple_domains_test"
./test & ./multiple_domains_test | diff - multiple_domains_test.out || echo "Test multiple_domains_test: failed!"
echo "Running say_recvcont_test"
./test & ./say_recvcont_test | diff - say_recvcont_test.out || echo "Test say_recvcont_test: failed!"
echo "Running disconnect_test"
./test & ./disconnect_test | diff - disconnect_test.out || echo "Test disconnect_test: failed!"
echo "Running pingpong_test (this one takes ~40s)"
./test & ./pingpong_test | diff - pingpong_test.out || echo "Test pingpong_test: failed!"

echo "Finished running all tests!"