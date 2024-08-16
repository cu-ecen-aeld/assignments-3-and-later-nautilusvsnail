1) Bring up a virtual machine host environment using Virtualbox and Ubuntu 20.04, or install Ubuntu 20.04 standalone on a dedicated host machine. See instructions here

 for help with this step.  Please ensure you use this version of Ubuntu, do not use the most recent version as this one has not yet been tested with the entirety of the course content.

2) Clone the github repository created from the github classroom link. This will create an empty repository. Don’t follow the instructions on the associated github page to add a README commit. Instead fill with template content using the git commands below inside the repository:

git remote add assignments-base https://github.com/cu-ecen-aeld/aesd-assignments.git

    This command specifies the base repository at https://github.com/cu-ecen-aeld/aesd-assignments

     with example starter code

git fetch assignments-base

    This command pulls the content locally to match the latest status at https://github.com/cu-ecen-aeld/aesd-assignments

     

git merge assignments-base/master

    This command makes your master branch match the master branch of   https://github.com/cu-ecen-aeld/aesd-assignments

git submodule update --init --recursive

    This command clones the assignment-autotest submodule

     and nested git repositories

Setup Your Development Host and Actions Runner:

3) See https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-Up-Your-Development-Host for the instructions to setup your Development Host.  See https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-Github-Actions

 to setup your Github Actions Runner.  You may use the same machine for your Development Host and Github Actions runner.
Run Unit Tests

4) Run the ./unit-test.sh script.  You must run this script from the base aesd-assignments directory. It will fail with a message like this:

    This is because the Unity test function in your repository within the student-test folder at Test_validate_username.c

     is not yet implemented.   

5) Update file conf/username.txt to include your github username (no leading/trailing spaces, no additional lines).

6) Implement the Test_validate_username.c

 function in your assignment repository.

7) Update your username in autotest-validate.c

.

8) Re-run the ./unit-test.sh script. The script should now get past the failure message above and the unit tests should now pass. This verifies the Unity unit test function stubs requirements.

9) Write a shell script finder-app/finder.sh as described below:

    Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir; the second argument is a text string which will be searched within these files, referred to below as searchstr

    Exits with return value 1 error and print statements if any of the parameters above were not specified

    Exits with return value 1 error and print statements if filesdir does not represent a directory on the filesystem

    Prints a message "The number of files are X and the number of matching lines are Y" where X is the number of files in the directory and all subdirectories and Y is the number of matching lines found in respective files, where a matching line refers to a line which contains searchstr (and may also contain additional content).

Example invocation:

       finder.sh /tmp/aesd/assignment1 linux

10) Write a shell script finder-app/writer.sh as described below

    Accepts the following arguments: the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile; the second argument is a text string which will be written within this file, referred to below as writestr

    Exits with value 1 error and print statements if any of the arguments above were not specified

    Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. Exits with value 1 and error print statement if the file could not be created.

Example:

       writer.sh /tmp/aesd/assignment1/sample.txt ios

Creates file:

    /tmp/aesd/assignment1/sample.txt

            With content:

            ios

11) Run the shell script finder-test.sh provided with your assignment github repository to test your implementations.  This script:

    Accepts the following arguments: the first argument is the number of files to write numfiles; the second argument is the string to write to each file writestr

    Defaults to use numfiles 10 and writestr “AELD_IS_FUN” if respective arguments are not specified.

    Creates an empty directory /tmp/aeld-data

    Loops to create numfiles files in directory /tmp/aeld-data using the writer.sh script, with writefile first argument name /tmp/aeld-data/usernameX.txt where X is a number from 1 to numfiles, username is the username found in conf/username.txt and writestr is passed as the second argument.

    Runs finder.sh script with filesdir set to /tmp/aeld-data and searchstr set to writestr

    Compares the output of the finder script with expected output “The number of files are numfiles and the number of matching lines are numfiles”. Prints “success” on match or “error” on mismatch.

12) Test your implementation using the shell script full-test.shand ensure all steps pass.

    This runs the same tests performed by Github Actions.