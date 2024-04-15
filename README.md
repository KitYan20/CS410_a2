# Assignment 2

### Group Members
Kit Yan and Mario Hysa

### Testing myshell
myshell behaves exactly like a normal Linux shell. To start the shell, compile the program in the terminal 
```bash
make myshell
```
Then, to enter myshell, you just need to run the executable in the terminal
```bash
./myshell
```
### Testing TAPPER
To test out the TAPPER program, you can run this command to compile all the required processes
```bash
make TAPPER
```
#### Testing TAPPER with synchronous buffering 
To test out TAPPER with synchronous buffering, run this command. Feel free to modify the optional argument value with the flag labeled "-o" or feel free to modify the buffer size. One thing we ran into when testing the class test file was that our buffer was too small to handle all the samples from the test file so it was stuck in a loop processing around 50 samples. So we figured that increasing the buffer size to 1000 would be the most ideal size for the buffer to process all the samples and it worked.
```bash
./tapper -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -o 2 -b sync -s 1000 < cs410-test-file > ipc-sync-cs410-test-file
```
#### Testing TAPPER with asynchronous buffering 
To test out TAPPER with asynchronous buffering, run this command. It would be the same way as synchronous buffering
```bash
./tapper -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -o 2 -b async < cs410-test-file > ipc-async-cs410-test-file
```
You will see all the resultant samples plotted by tapplot be redirected to it's corresponding output file

### Testing TAPPET
To test out the TAPPET program, you can run this command to compile the tappet executable and shared library

```bash
make TAPPET
```
Make sure that when you are running the test, set the LD_LIBRARY_PATH to where the shared libary is located in you directory

Here is how you can do it:
```bash
LD_LIBRARY_PATH=./
```
```bash
export LD_LIBRARY_PATH
```

#### Testing TAPPET with synchronous buffering 
```bash
./tappet -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -o 2 -b sync -s 1000 < cs410-test-file > thread-sync-cs410-test-file
```
#### Testing TAPPET with asynchronous buffering 

```bash
./tappet -p1 ./observe -p2 ./reconstruct -p3 ./tapplot -o 2 -b async < cs410-test-file > thread-async-cs410-test-file
```
### Automated compiling and removing program
To compile Tapper with all the required processes, run this command
```bash
make TAPPER
```
To compile Tapper with the required tappet executable and the shared library containing object files for observe, reconstruct, tapplot, run this command
```bash
make TAPPET
```
To remove all the files, run this command
```bash
make clean
```
### General Info about the assignment