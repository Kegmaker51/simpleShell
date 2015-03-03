# simpleShell
A simple shell implemented in C (C99)
Completed For CS243 @ RIT

DESIGN:

myshell handles user input through getopt and getline. I use a loop in
conjunction with getopt to make sure that all of the arguments passed
at the startup of the shell are correct. From there getline is used
to pull the lines that the user types into stdin so that they may then
be parsed. Once parsed the passed in command is then check against
a list of internal commands. If it is an internal command the
appropriate function is then called; if it is not then the shell
attempts to fork and exec the command. 

LIMITATIONS:

My shell is a bit heavy on the memory usage end of things. It also
can only handle commands of up to 1024 (Not including null terminating
character) characters. It also cannot handle the following number of
history items SO PLEASE DO NOT USE THEM OR EVERYTHING WILL BREAK:
9,8,7,5,4,and sometimes 3.
