/*mysh.c
 *
 *Author: Alexander R. Cavaliere <arc6393@rit.edu>
 *
 *My shell implementation; the shell can handle 5 internal commands
 *(Bang, Help, History, Quit, and Verbose!) and UNIX commands via fork, exec,
 * and wait.
 *
 *Version:
 * $Id: mysh.c,v 1.8 2014/12/12 03:55:02 arc6393 Exp $
 */

#define _GNU_SOURCE
#define ALLOC 3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>


//GLOBALS
typedef struct historyList
{
    char **commandHistory;
    int commands;
    int commandHistoryMem;
} History;

/*mysh_bang
 *
 * The internal command that reads from the command history list
 * and reruns the Nth command (As specified by the user). If the
 * command is not within the history value limit then the user
 * will be informed that the command they are looking for does not
 * exist.
 *
 * @params argc The verbose flag; used to print extra information
 * to stdout
 * @params argv The history struct; contains the command list and
 * the history value.
 * @return 0 Upon success
 * @return 1 Upon failure
 * @return argc If verbose is called again (Toggles vmode)
 *  */
int mysh_bang(int argc, char * argv[])
{
    History * holder = (History *)argv;
    int value = ((History *)argv)->commands - ((History *)argv)->commandHistoryMem;
    if(argc)
    {
        printf("     COMMAND: bang => processing!\n");
    }
    int distance = 0;
    char * inter;
    if(holder->commands > holder->commandHistoryMem)
    {
        inter = (char *) malloc(sizeof(char *) * strlen(
                holder->commandHistory[holder->commandHistoryMem-1]) + 1);

        for(int i = 1; i < (strlen(holder->commandHistory[
                holder->commandHistoryMem-1]) + 1); i++)
        {
            inter[i-1] = holder->commandHistory[holder->commandHistoryMem-1][i];
        }
    }
    else
    {
        inter = (char *) malloc(sizeof(char *) * strlen(
                holder->commandHistory[holder->commands - 1]) + 1);

        for(int i = 1; i < (strlen(holder->commandHistory
        [holder->commands-1]) + 1); i++)
        {
            inter[i-1] = holder->commandHistory[holder->commands-1][i];
        }
    }
    sscanf(inter, "%d", &distance);
    if(distance < ((History *)argv)->commands - ((History *)argv)->commandHistoryMem
            || distance == ((History *)argv)->commands)
    {
        return 1;
    }
    else
    {
        char * command;
        char * backup;
        if(distance < holder->commands)
        {
            command = (char *) malloc(sizeof(char *) * strlen(
                    holder->commandHistory[distance-1])+1);

            backup = (char *) malloc(sizeof(char *) * strlen(
                    holder->commandHistory[distance-1])+1);

            strcpy(backup,holder->commandHistory[distance]);
            sscanf(holder->commandHistory[distance],"%s",command);
        }
        else
        {
            command = (char *) malloc(sizeof(char *) * strlen(
                    holder->commandHistory[distance - value -1])+1);

            backup = (char *) malloc(sizeof(char *) * strlen(
                    holder->commandHistory[distance - value - 1])+1);

            strcpy(backup, holder->commandHistory[distance - value]);
            sscanf(holder->commandHistory[distance - value], "%s", command);
        }
        if(!strcmp(command,"help"))
        {
            free(inter);
            free(command);
            mysh_help(argc, argv);
        }
        else if(!strcmp(command,"history"))
        {
            free(inter);
            free(command);
            mysh_history(argc, argv);
        }
        else if(!strcmp(command,"verbose"))
        {
            free(inter);
            free(command);
            argc = mysh_verbose(argc, argv);
            return argc;
        }
        else
        {
            free(inter);
            free(command);
            char * arguments[1024];
            char ** nextCommand = arguments;
            char * temp = strtok(backup ," \n");
            while (temp != NULL)
            {
                *nextCommand++ = temp;
                temp = strtok(NULL, " \n");
            }

            pid_t pid;
            pid_t usefulInfo = 0;
            int status = 0;
            if (argc)
            {
                int counter = 0;
                printf("     Input command tokens:\n");
                for (nextCommand = arguments; *nextCommand != 0; nextCommand++)
                {
                    printf("%d:", counter);
                    puts(*nextCommand);
                    counter++;
                }
            }

            //Begin Fork and Exec Block
            if ((pid = fork()) < 0)
            {
                perror("Fork error; something has gone wrong!");
            }
            else if (pid > 0) //THIS IS THE PARENT
            {
                usefulInfo = wait(&status);
                if (argc)
                {
                    printf("     Parent waited on pid: %d\n", usefulInfo);
                }
                free(backup);

            }
            else //THIS IS THE CHILD
            {
                execvp(arguments[0], arguments);
                fprintf(stderr, "     %s: No such file or directory\n", command);
                fprintf(stderr, "     command status: %d\n", status);
                mysh_quit(argc,  argv);
                free(backup);
                _exit(EXIT_FAILURE);
            }//End Fork and Exec Block
            memset(arguments, 0, sizeof(arguments));
        }
        return 0;
    }

}//end mysh_bang

/*mysh_help
 *
 *The internal help function for mysh. When called it prints text explaining
 *how to use each of the internal commands as well as what each command does
 *
 * @params argc Used to pass in the verbose flag (Not used in this function
 * @params argv Used to pass in the history struct (Not used in this function)
 * @return 0 Signals the success of the help command
 */
int mysh_help(int argc, char * argv[])
{
    if(argc)
    {
        printf("     COMMAND: help => processing!");
    }
    printf("Internal Commands:\n");
    printf("!N:      Rexecute the Nth command in the history list where N is a \n");
    printf("         positive integer.\n");
    printf("help:    Outputs this text.\n");
    printf("history: Outputs the list of commands entered. Only 'remembers' a \n");
    printf("         certain number of commands.The value can be set when first \n");
    printf("         running the shell using the -h flag and specifying a  \n");
    printf("         positive integer afterwards. The default integer is 10. \n");
    printf("quit:    Deallocs all memory in use by the shell and then cleanly \n");
    printf("         terminates the shell.\n");
    printf("verbose: Toggle verbose mode in the shell. Can be set when the shell \n");
    printf("         is first run by using the -v flag. Verbose takes 'on' or  \n");
    printf("         'off' as arguments.\n");
    return 0;
}//end mysh_help

/*mysh_history
 *
 * Returns the list of commands that the user entered in to the terminal up
 * to a certain amount (Defined by the user or 10 by default).
 *
 * @params argc The verbose flag; used for print extra information to stdout
 * @params argv The history struct; contains the command list and the history
 * value
 * @return 0 Upon success
 */
int mysh_history(int argc, char * argv[])
{
    if(argc)
    {
        printf("     COMMAND: history => processing!");
    }
    History * holder = ((History *)argv);
    int limit = holder->commandHistoryMem;
    int formater = limit;
    int commands = holder->commands;

    if(commands < limit)
    {
        for(int i = 0; i < commands; i++)
        {
            printf("%d: %s", i, holder->commandHistory[i]);
        }
    }
    else
    {
        for (int i = 0; i < limit; i++) {
            printf("%d: %s", commands - formater, holder->commandHistory[i]);
            formater--;
        }
    }
    return 0;
}//end mysh_history

/*mysh_quit
 *
 * Frees the history struct and all of the command strings within its list.
 * It then signals for the termination of the shell upon it's success.
 *
 * @params argc The verbose flag; used to print extra information to stdout
 * @params argv The history struct; contains the command list which must be
 * freed along with the struct itself
 * @return 0 Upon success
 */
int mysh_quit(int argc, char * argv[])
{
    if(argc)
    {
        printf("     COMMAND: quit => processing!\n");
    }

    for(int i = 0; i < ((History *)argv)->commandHistoryMem; i++)
    {
        free(((History *)argv)->commandHistory[i]);
    }
    free(((History *)argv)->commandHistory);
    free((History *)argv);
    return 0;
}//end mysh_quit

/*verbose
 *
 * Turns verbose mode on or off. Verbose mode on prints additional
 * information to stdout. Off leaves all of the extra info off.
 *
 * @return 1 If user wants vmode on
 * @return 0 If user wants vmode off
 * @return argc Something went wrong (So change nothing!)
 */
int mysh_verbose(int argc, char * argv[])
{
    if(argc)
    {
        printf("     COMMAND: verbose => processing!\n");
    }
    History * holder = (History *)argv;
    int commands = holder->commands;
    int limit = holder->commandHistoryMem;
    char * verb;
    char * on;
    if(commands > limit)
    {
        verb = (char *) malloc(sizeof(char *) * strlen(
                holder->commandHistory[limit - 1]) + 1);

        on = (char *) malloc(sizeof(char *) * 4);
        sscanf(holder->commandHistory[limit-1], "%s %s",verb, on);
    }
    else
    {
        verb = (char *) malloc(sizeof(char *) * strlen(
                holder->commandHistory[commands-1]) + 1);

        on = (char *) malloc(sizeof(char *) * 4);
        sscanf(holder->commandHistory[commands-1], "%s %s",verb, on);
    }
    if(!strcmp(on, "on"))
    {
        free(verb);
        free(on);
        return 1;
    }
    else if(!strcmp(on, "off"))
    {
        free(verb);
        free(on);
        return 0;
    }
    return argc;
}//end mysh_verbose


/*main
 *
 * The main loop that checks for user input to determine what to do next.
 * It uses getopt to make sure that the appropriate CL arguments are being fed
 * to the program. Shell will quit if those arguments are invalid. Afterwards
 * user input is taken in via getline (from unistd.h); it handles the reallocation
 * of memory for the input string. That string is then parsed and checked against
 * the internal commands. If the input is an internal command that command is then
 * run by the shell and it then returns to the loop. If the command is an external
 * command the program forks and execs it; returning whatever exec returns in case
 * of error or executing the command upon success.
 *
 * The history size is variable but the following numbers are anathema to the shell:
 * 9, 8, 7, 5, 4, and (Sometimes) 3. Basically most things lower than the default
 * cause problems.
 *
 * @params argc Number of CL arguments
 * @params argv The CL arguments
 * @return Various Ints Depends on the success or failure of a command
 */
int main(int argc, char * argv[])
{
    int verboseFlag = 0;
    int historyFlag = 0;
    char * historyValue = NULL;
    int success;
    char *incomingCommand = (char *) malloc(sizeof(char *) * ALLOC);
    size_t incomingCommandBytes = ALLOC;
    opterr = 0;
    History * commandHistoryMaster = (History *) malloc(sizeof(History));
    commandHistoryMaster->commandHistory;
    commandHistoryMaster->commands = 0;
    commandHistoryMaster->commandHistoryMem = 10;

    //Time to get the user's arguments!
    while((success = getopt(argc, argv, "vh:")) != -1)
    {
        switch(success)
        {
            case 'v':
                verboseFlag = 1;
                break;
            case'h':
                historyFlag = 1;
                historyValue = optarg;
                int temporaryInt = 0;
                sscanf(historyValue,"%d",&temporaryInt);
                if(temporaryInt <= 0)
                {
                    fprintf(stderr, "usage: mysh [-v] [-h pos_num]\n");
                    return 1;
                }
                break;
            case '?':
                if(isprint (optopt))
                {
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                    return 1;
                }
                else
                {
                    fprintf(stderr, "Uknown option character '\\x%x'.\n", optopt);
                    return 1;
                }
            default:
                abort();
        }
    }//All done! (End argument parsing while loop)

    if(historyFlag)
    {
        int temporaryInt = 0;
        sscanf(historyValue,"%d",&temporaryInt);
        commandHistoryMaster->commandHistoryMem = temporaryInt;
    }

    commandHistoryMaster->commandHistory = (char **) malloc(sizeof(char **) *
            commandHistoryMaster->commandHistoryMem);

    printf("mysh[%d]>",commandHistoryMaster->commands);

    //Time to run commands!
    while((getline(&incomingCommand, &incomingCommandBytes, stdin)) != EOF)
    {
        char * command = (char *) malloc(sizeof(char*) * 7);
        sscanf(incomingCommand,"%s ",command);

        if(incomingCommand[0] == '\n' || (int)incomingCommand[0] == 32)
        {
            printf("mysh[%d]>",commandHistoryMaster->commands);
            continue;
        }

        //Verbose Check
        if(verboseFlag)
        {
            printf("     Command Entered (Verbatim): %s", incomingCommand);
            printf("     Command (Arguments Stripped): %s\n", command);
        }

        if(command[0] == '!') //BANG COMMAND
        {
            if(commandHistoryMaster->commandHistoryMem <= commandHistoryMaster->commands)
            {
                for(int i = 0; i < commandHistoryMaster->commandHistoryMem; i++)
                {
                    commandHistoryMaster->commandHistory[i] =
                            commandHistoryMaster->commandHistory[i+1];
                    if(i == commandHistoryMaster->commandHistoryMem - 1)
                    {
                        break;
                    }
                }
                free(commandHistoryMaster->commandHistory[
                        commandHistoryMaster->commandHistoryMem-1]);

                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1] = (char *)
                        malloc(sizeof(char *) * (strlen(incomingCommand) + 1));

                strcpy(commandHistoryMaster->commandHistory
                        [commandHistoryMaster->commandHistoryMem-1],
                        incomingCommand);

                commandHistoryMaster->commands++;
                mysh_bang(verboseFlag, (char **)commandHistoryMaster);
            }
            else
            {
                commandHistoryMaster->commandHistory[commandHistoryMaster->commands] =
                        (char *) malloc(sizeof(char *) * strlen(incomingCommand) + 1);
                strcpy(commandHistoryMaster->commandHistory[
                        commandHistoryMaster->commands],incomingCommand);

                commandHistoryMaster->commands++;

                mysh_bang(verboseFlag,((char **)commandHistoryMaster));
            }
        }

        else if(!strcmp(command,"help")) //HELP COMMAND
        {
            if(commandHistoryMaster->commandHistoryMem <= commandHistoryMaster->commands)
            {
                for(int i = 0; i < commandHistoryMaster->commandHistoryMem; i++)
                {
                    commandHistoryMaster->commandHistory[i] =
                            commandHistoryMaster->commandHistory[i+1];

                    if(i == commandHistoryMaster->commandHistoryMem - 1)
                    {
                        break;
                    }
                }
                free(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1]);

                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1] = (char *)
                        malloc(sizeof(char *) * (strlen(incomingCommand) + 1));

                strcpy(commandHistoryMaster->commandHistory
                        [commandHistoryMaster->commandHistoryMem-1],
                        incomingCommand);

                commandHistoryMaster->commands++;
                mysh_help(verboseFlag, NULL);
            }
            else
            {
                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands] =
                        (char *) malloc(sizeof(char *) * strlen(incomingCommand) + 1);

                strcpy(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands], incomingCommand);

                commandHistoryMaster->commands++;

                mysh_help(verboseFlag, NULL);
            }
        }

        else if(!strcmp(command,"history")) //HISTORY COMMAND
        {
            if(commandHistoryMaster->commandHistoryMem <= commandHistoryMaster->commands)
            {
                for(int i = 0; i < commandHistoryMaster->commandHistoryMem; i++)
                {
                    commandHistoryMaster->commandHistory[i] =
                            commandHistoryMaster->commandHistory[i+1];

                    if(i == commandHistoryMaster->commandHistoryMem - 1)
                    {
                        break;
                    }
                }
                free(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1]);

                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1] = (char *)
                        malloc(sizeof(char *) * (strlen(incomingCommand) + 1));

                strcpy(commandHistoryMaster->commandHistory
                        [commandHistoryMaster->commandHistoryMem-1],
                        incomingCommand);

                commandHistoryMaster->commands++;
                mysh_history(verboseFlag, (char **)commandHistoryMaster);
            }
            else
            {
                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands] =
                        (char *) malloc(sizeof(char *) * strlen(incomingCommand) + 1);

                strcpy(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands],incomingCommand);
                commandHistoryMaster->commands++;

                mysh_history(verboseFlag,((char **)commandHistoryMaster));
            }


        }

        else if(!strcmp(command,"quit")) //QUIT COMMAND
        {
            int quit = mysh_quit(verboseFlag, (char **)commandHistoryMaster);
            if(!quit)
            {
                free(incomingCommand);
                free(command);
                return 0;
            }
        }

        else if(!strcmp(command,"verbose")) //VERBOSE COMMAND
        {
            if(commandHistoryMaster->commandHistoryMem <= commandHistoryMaster->commands)
            {
                for(int i = 0; i < commandHistoryMaster->commandHistoryMem; i++)
                {
                    commandHistoryMaster->commandHistory[i] =
                            commandHistoryMaster->commandHistory[i+1];

                    if(i == commandHistoryMaster->commandHistoryMem - 1)
                    {
                        break;
                    }
                }
                free(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1]);

                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1] = (char *)
                        malloc(sizeof(char *) * (strlen(incomingCommand) + 1));

                strcpy(commandHistoryMaster->commandHistory
                        [commandHistoryMaster->commandHistoryMem-1],
                        incomingCommand);

                commandHistoryMaster->commands++;

                verboseFlag = mysh_verbose(verboseFlag, (char **)commandHistoryMaster);
            }
            else
            {
                commandHistoryMaster->commandHistory[commandHistoryMaster->commands] =
                        (char *) malloc(sizeof(char *) * strlen(incomingCommand) + 1);

                strcpy(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands],incomingCommand);
                commandHistoryMaster->commands++;

                verboseFlag = mysh_verbose(verboseFlag,((char **)commandHistoryMaster));
            }
        }

        else //MOST GLORIOUS EXTERNAL COMMANDS GO HERE
        {
            if(commandHistoryMaster->commandHistoryMem <= commandHistoryMaster->commands)
            {
                for(int i = 0; i < commandHistoryMaster->commandHistoryMem; i++)
                {
                    commandHistoryMaster->commandHistory[i] =
                            commandHistoryMaster->commandHistory[i+1];

                    if(i == commandHistoryMaster->commandHistoryMem - 1)
                    {
                        break;
                    }
                }
                free(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1]);

                commandHistoryMaster->commandHistory
                [commandHistoryMaster->commandHistoryMem-1] = (char *)
                        malloc(sizeof(char *) * (strlen(incomingCommand) + 1));

                strcpy(commandHistoryMaster->commandHistory
                        [commandHistoryMaster->commandHistoryMem-1],
                        incomingCommand);

                commandHistoryMaster->commands++;
            }
            else
            {
                commandHistoryMaster->commandHistory[commandHistoryMaster->commands] =
                        (char *) malloc(sizeof(char *) * strlen(incomingCommand) + 1);

                strcpy(commandHistoryMaster->commandHistory
                [commandHistoryMaster->commands],incomingCommand);

                commandHistoryMaster->commands++;
            }

            char * arguments[1024];
            char ** nextCommand = arguments;
            char * temp = strtok(incomingCommand ," \n");
            while (temp != NULL)
            {
                *nextCommand++ = temp;
                temp = strtok(NULL, " \n");
            }

            pid_t pid;
            pid_t usefulInfo = 0;
            int status = 0;
            if (verboseFlag)
            {
                int counter = 0;
                printf("     Input command tokens:\n");
                for (nextCommand = arguments; *nextCommand != 0; nextCommand++)
                {
                    printf("%d:", counter);
                    puts(*nextCommand);
                    counter++;
                }
            }

            //Begin Fork and Exec Block
            if ((pid = fork()) < 0)
            {
                perror("Fork error; something has gone wrong!");
            }
            else if (pid > 0) //THIS IS THE PARENT
            {
                usefulInfo = wait(&status);
                if (verboseFlag) {
                    printf("     Parent waited on pid: %d\n", usefulInfo);
                }

            }
            else //THIS IS THE CHILD
            {
                execvp(arguments[0], arguments);
                fprintf(stderr, "     %s: No such file or directory\n", command);
                fprintf(stderr, "     command status: %d\n", status);
                mysh_quit(verboseFlag, (char **) commandHistoryMaster);
                free(incomingCommand);
                free(command);
                _exit(EXIT_FAILURE);
            }//End Fork and Exec Block
            memset(arguments, 0, sizeof(arguments));
        }
        free(command);
        printf("mysh[%d]>",commandHistoryMaster->commands);
    }
    mysh_quit(verboseFlag, (char **)commandHistoryMaster);
    free(incomingCommand);
    return 0;
}//end main

/*Revisions:
 *$Log: mysh.c,v $
 *Revision 1.8  2014/12/12 03:55:02  arc6393
 *Eight Commit:
 *=>Final Version
 *=>Ship it out to prod boys!
 *
 *Revision 1.7  2014/12/12 02:07:49  arc6393
 *Seventh Commit:
 *=>External Commands work!
 *=>Working on bang
 *=>Need to start verbose
 *
 *Revision 1.6  2014/12/11 08:21:01  arc6393
 *Sixth Commit
 *=>Updated the string passage
 *=>Removed erronious allocations of mem
 *=>Need to put in checks for realloc
 *=>history works
 *=>need to test bang
 *=>need to implement verbose
 *=>quit, help, and ext commands all work
 *=>valgrind is the best tool ever
 *
 *Revision 1.5  2014/12/10 18:50:06  arc6393
 *Fifth Commit:
 *=>Minor editting changes as I hunt down the issues within!
 *
 *Revision 1.4  2014/12/10 04:53:48  arc6393
 *Fourth Commit:
 *=>Very close to having external commands working flawlessly
 *=>Something's going on with the realloc size; it crashes
 * everytime at 20 commands (without fail). Need to put in
 * print statements to make sure everything is working as it should be.
 *
 *Revision 1.3  2014/12/09 03:52:05  arc6393
 *Third Commit:
 *=>Completed External Command handling
 *=>Completed mysh_help()
 *=>Need to remember to free up arguments array; it knows too much!
 *
 *Revision 1.2  2014/12/09 01:49:34  arc6393
 *Second Commit:
 *=>Handled the need to realloc
 *=>Created a struct to handle the history
 *=>Pass struct around as char** to get around silly restrictions
 *
 */