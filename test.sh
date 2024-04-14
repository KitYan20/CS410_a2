#uncomment to test each out
#Sync with our own test file
make TAPPER;./tapper -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -b sync -s 10 < test_file.txt;make clean_tapper
#Async with our own test file but tapplot does not work for now for async
make TAPPER;./tapper -p1 ./observe -p2 ./reconstruct -o 2 -b async -s 10 < test_file.txt;make clean_tapper
# testing with async process tapplot does not work for now for async

#TESTING WITH cs410-test-file
#Sync probably still not working but feel free to test
make TAPPER; ./tapper -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -o 2 -b sync -s 10 < cs410-test-file;make clean_tapper

#Line below to test. To better debug, I would comment out line 165-177 in reconstruct.c and just test it out to see if it processed all the constructed samples
make TAPPER; ./tapper -p1 ./observe -p2 ./reconstruct -o 2 -b async -s 10 < cs410-test-file;make clean_tapper

#Async is also bit wonky when processing the reconstructed samples for the 616 samples test file
make TAPPER; ./tapper -p1 ./observe -p2 ./reconstruct -o 2 -b async -s 10 < cs410-test-file;make clean_tapper

#COMMENT OUT THE ONES YOU WOULD NOT USE!!