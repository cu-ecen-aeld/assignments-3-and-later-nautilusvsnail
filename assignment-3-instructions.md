Implementation:

    Modify your finder-app/finder-test.sh script to remove the make step.

        You will add a cross-compile make step for this utilty in a different script as a part of Assignment 3 part 2.

    Make modifications in the examples/systemcalls/systemcalls.c

 file to implement the TODO there related to video content and system() and exec() functions.  See provided test code in https://github.com/cu-ecen-aeld/assignment-autotest/blob/master/test/assignment3/Test_systemcalls.c

 which will verify your implementation.  Run ./unit-test.sh to test your implementation using unity unit tests.

Tag your repository assignment-3-part-1 using https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Tagging-a-Release

     

Validation:

Your unit-test.sh script should pass against your systemcalls implementation.  Note that your github actions will fail due to full-test.sh, we will add support for this in part 2 of the assignment.
Troubleshooting:

You may notice printf() output duplicated after your fork() call.  Use fflush(stdout) before the fork() call to avoid duplicate prints.  See https://stackoverflow.com/questions/42690197/why-does-this-program-with-fork-print-twice/42690260#42690260
 for details.