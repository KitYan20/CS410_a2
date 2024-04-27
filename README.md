# Assignment 2

### Group Members
Kit Yan and Mario Hysa

### Testing myshell
We've tested out both TAPPER and TAPPET programs in myshell and it seems to run gracefully without redirection to a output file for some reason when I'm testing it with synchronous buffering for both TAPPER and TAPPET. So for now, you can testing out the test command line if you are testing async on myshell and test sync on a normal shell if you are redirecting the resultant samples to file. If not, you can run sync on myshell. myshell behaves exactly like a normal Linux shell. To start the shell, compile the program in the terminal 
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
To test out TAPPER with synchronous buffering, run this command. Feel free to modify the optional argument value with the flag labeled "-o" or feel free to modify the buffer size. One thing we ran into when testing the class test file was that our buffer was too small to handle all the samples from the test file so it was stuck in a loop processing around 50 samples. So we figured that increasing the buffer size to 1000 would be the most ideal size for the buffer to process all the samples and it worked. If you want to test synchronous with redirection to a output file, make sure to run it on a normal shell because for some reason, it crashes. But it won't crash if you are not do any stdout redirection to a file. Same goes for TAPPET.
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
To test out the TAPPET program, you can run this command to compile the tappet executable and it's shared library
to access all the functions needed for tappet.c to use for multi-threading.

```bash
make TAPPET
```
Make sure that when you are running the test, set the LD_LIBRARY_PATH to where the shared libary is located in you directory. And also comment out the main function in observe, reconstruct, and tapplot as it causes an error of multiple declaration of the main function when linking.

Here is how you can do it:
```bash
LD_LIBRARY_PATH=./
```
```bash
export LD_LIBRARY_PATH
```

#### Testing TAPPET with synchronous buffering (Same process as TAPPER)
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

This assignment in general was quite a challenging task. It felt grateful that Professor West was able to extend the deadline to Tuesday because I wouldn't have finished it if he hadn't. Overall, we managed to get the functionalities working on myshell as both TAPPER and TAPPET programs run on it without any problems. We felt that the most diffcult part of this assignment was understanding why TAPPER was working with our test file that we created based on the Assignment example and that was able to run successfully. However, when we went to test the class file, it wasn't exiting out of the process and it took us some time to find out that the buffer size had to increase in order for read the data in reconstruct. Another issue that we also ran into was not understanding the full logic of why the buffers would be stuck in a deadlock even though the logic made sense. We took the pseudocode logic from Anton's ring buffer web link and implement on our code to get it working. We just needed to add a few more conditions in order to fully get the data sent from one process to another for TAPPER. The other issue we ran into was that async processing in reconstruct producing the reconstructed samples to tapplot consume wasn't working as it was stuck in a deadlock. But observe to reconstruct was working fine which we didn't know why as the logic would just be the same as that. So due to time constraint and the risk of making any changes that would ruin the rest of the code, we decided to just graph the reconstructed samples from reconstruct and let tapplot plot the samples from the reconstructed.txt file. Other than that TAPPET was fairly easy to implement as we follow the logic of tapper.c parsing the command line for arguments and then using pthreads.h library to use the required functions for multithreading. To ease our life of just copying all the functions from observe, reconstruct, tapplot, we implemented a dynamic library to get the address of the symbol for each file in the dynamic library of object files. Overall, the assignment was quite challenging because we were running into issues that we didn't know how to fix so we did a bit of researching in order to know what was wrong. But it was definitely insightful and it taught us a lot of new concepts of what IPC is about.
