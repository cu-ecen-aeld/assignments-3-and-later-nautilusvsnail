Setup your assignment repository using the github classroom instructions and link above.  You should see your previous assignment content in the repository linked from the github classroom link at the top of the document when you complete these steps.

Setup an ARM cross compile toolchain as described in Module 1 instructions and the wiki page at https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Installing-an-ARM-aarch64-developer-toolchain

     

    Create an assignments/assignment2/cross-compile.txt file in your repository showing version, configuration and sysroot path of aarch64-none-linux-gnu-gcc using -print-sysroot  and -v options in an file you add for validation purposes. 

        There are no requirements about how you create or fill this file and you may simply copy and paste from command line output.  If you chose to use redirection please note that some of the content is written to stderr and you must redirect stderr to the file.  See  https://stackoverflow.com/a/876242

         .

3. Write a C application “writer” (finder-app/writer.c)  which can be used as an alternative to the “writer.sh” test script created in assignment1 and using File IO as described in LSP chapter 2.  See the Assignment 1 requirements for the writer.sh test script and these additional instructions:

    One difference from the write.sh instructions in Assignment 1:  You do not need to make your "writer" utility create directories which do not exist.  You can assume the directory is created by the caller.

    Setup syslog logging for your utility using the LOG_USER facility.

    Use the syslog capability to write a message “Writing <string> to <file>” where <string> is the text string written to file (second argument) and <file> is the file created by the script.  This should be written with LOG_DEBUG level.

    Use the syslog capability to log any unexpected errors with LOG_ERR level.

4. Write a Makefile which includes:

    A default target which builds the “writer” application

    A clean target which removes the “writer” application and all .o files

    Support for cross-compilation.  You should be able to generate an application for the native build platform when GNU make variable CROSS_COMPILE is not specified on the make command line.  When CROSS_COMPILE is specified with aarch64-none-linux-gnu- (note the trailing -)your makefile should compile successfully using the cross compiler installed in step 1.

5. Make modifications to the finder-app/finder-test.sh script provided with the assignment github repository as described below.

    Clean any previous build artifacts.

    Compile your writer application using native compilation

    Use your “writer” utility instead of “writer.sh” shell script.

6. Verify your finder-app/finder-test.sh script works with your writer application instead of writer.sh

7. Test your implementation using the shell script ./full-test.sh and ensure all scripts pass.

    This verifies the writer and tester.sh implementation passes requirements.

8. Verify the “file” utility (https://man7.org/linux/man-pages/man1/file.1.html

) indicates an aarch64 executable type when building with CROSS_COMPILE specified on the command line.

    Include the output of the “file” utility after building writer with CROSS_COMPILE in an “assignments/assignment2/fileresult.txt” file for grading purposes.

Validation/Deliverables:

    Your assignments/assignment2/cross-compile.txt file should show the version, configuration and sysroot path (output of -print-sysroot)

    The finder-test.sh script should return “success” when run.

    Your writer application should meet requirements from assignment 1 regarding error handling.

    Ensure all error handling has been implemented for writer.c.

    Ensure syslog logging is setup and working properly (you should see messages logged to /var/log/syslog on your Ubuntu VM).

    If using Windows Subsystem for Linux, you may need to manually start rsyslog with `sudo service rsyslog start`

5. Your assignments/assignment2/fileresult.txt should show that you were able to cross compile successfully

6. Your github actions automated test script should pass on your repository and the “Actions” tab should show a successful run on your last commit.