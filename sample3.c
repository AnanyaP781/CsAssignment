#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

// Constants
#define MAX_ARG 15
#define MAX_LINE 80
#define MAX_HISTORY 10
#define MAX_LIST 100

// Functions Prototype
char * inputtext();
char ** parsetext(char * input);
char ** executeshell(char * instruction);
void execute(char *arg, char **args);
void historyfeature(char * instruction,int featurenum);
char* returninstruction(int x);
int processString(char* str, char** parsed, char** parsedpipe);
void execArgsPiped(char** parsed, char** parsedpipe);

//Global Variable and Arrays used for program
int sizeofcmd, numarg=0, currenthistory=-1;
char running = 1, tempbool=0, bls=0, ready=0;
char * history[MAX_HISTORY];
char **cmdpnt;

// Main Program
int main()
{
    char* input;
    char *parsedArgs[MAX_LIST], *parsedArgsPiped[MAX_LIST];
    int i, exeflag = 0;
    char** commands;
    // Loop running the program
    while(running)
    {
        input = inputtext();
        if(input[0]!='\n')
        {    input = strtok(input,"\n\r");
             commands  = executeshell(input);
             // Executing the commands
	    if(ready)       
            {        
                exeflag = processString(input, parsedArgs, parsedArgsPiped);
                /* 0 for no command or built in command
                   1 for simple command
                   2 for piped command */

                   if (exeflag==1)
                     execute(commands[0],commands);

                   if (exeflag==2)
                     execArgsPiped(parsedArgs, parsedArgsPiped);
                       
		        ready=0;
	            free(commands);
           
            }
        
        if(bls)
            free(cmdpnt),bls=0;
        
        numarg = 0;
	
        
	    }  	
	
    }
   
    return 0;
}

// Function that inputs the whole instruction
char * inputtext()
{    char *buffer = NULL;
    size_t x;
    printf("osh> ");
    fflush(stdout);
    sizeofcmd = getline(&buffer, &x, stdin);
    if(sizeofcmd == -1 && sizeofcmd < MAX_LINE)
        printf("Error Reading the Line \n");
    return buffer;
}

// Function that tokenizes the instruction into several independent words
char ** parsetext(char * input)
{
    char **cmd;
    cmd = malloc(MAX_ARG* sizeof(char *));
    char *temp;
    temp = strtok(input," \t\r\a\n");
    while(temp != NULL && numarg<=MAX_ARG)
    {
        cmd[numarg++]=temp;
        temp = strtok(NULL, " \t\r\a\n");
    }
   
    return cmd;
}

// Function Responsible for the shell events
char ** executeshell(char * instruction)
{    char *tempstring = malloc((strlen(instruction))*sizeof(char));
    strcpy(tempstring,instruction);
   
    char ** commands  = parsetext(tempstring);
     if(commands[0] != NULL)
    {
       if(!strcmp(commands[0], "history"))   
        free(tempstring),historyfeature(instruction ,0),ready=0;
       else if(!strcmp(commands[0], "!!"))
        free(tempstring), historyfeature(instruction ,1),ready=0;

       else if(commands[0][0]=='!' && isdigit(commands[0][1]))
        free(tempstring), historyfeature(instruction ,2),ready=0;
       else if(!strcmp(commands[0], "exit"))
        running=0, ready=0;      
       else {
        historyfeature(instruction ,3);
        ready=1;
        }              
    }
    return commands;
     
}

// Function that executes the commands using forks
void execute(char *arg, char **args)
{
   
        if (!strcmp(args[numarg-1],"&"))    // Concurrency Test
            tempbool=1, args[numarg-1]=NULL;

        pid_t pid = fork();
           
        if (pid < 0) 
        {                 // Fork Failure
            fprintf(stderr, "Fork Failed\n");
       
        }
        else if (pid == 0) 
        {            // Fork Success Child
            if(execvp(arg,args)==-1)
                { printf("Invalid Command\n");
		          exit(0);
                }
        }
        else                     // Parent
        {    if(tempbool)   
                wait(NULL), tempbool=0;
        }
   
   
}

// Function responsible for the history
void historyfeature(char * instruction, int featurenum)
{
    
	
    int i;
    char * tempstring;
    if(featurenum==0)        // Printing the history
        {
        if(currenthistory !=-1)
          {  for(i=0;i<currenthistory+1;i++)
                printf("%d: %s \n",i,history[i]);
	  }
        else printf("No Commands in History 0\n");
        }           
         
    else if(featurenum==1)        // !! Functionality
        {
        if(currenthistory !=-1)
            {tempstring = returninstruction(currenthistory);
            numarg = 0;
            cmdpnt = parsetext(tempstring);   
            execute(cmdpnt[0],cmdpnt);
            free(tempstring);
            bls=1;
            }
        else printf("No Commands in History 1\n");
           
        }
   
    else if(featurenum==2)        // !digit Functionality
        {if(currenthistory !=-1)
            {int x =instruction[1]-'0';
           
            if(x>=0 && x<=currenthistory)
            {
                tempstring = returninstruction(x);
                numarg = 0;
                cmdpnt = parsetext(tempstring);
                execute(cmdpnt[0],cmdpnt);
                free(tempstring);
                bls=1;
           
            }
            else printf("No Such Commands in History\n");
            }
        else printf("No Commands in History 2\n");
        }

    else if(featurenum==3)    // Saving in history functionality
        {    if(currenthistory!=MAX_HISTORY-1)
            {    currenthistory=currenthistory +1;
                history[currenthistory] = instruction;
            }
            else if(currenthistory==MAX_HISTORY-1)
                {    int x;
                    for(x=1;x<MAX_HISTORY;x++)
                        history[x-1] = history[x];
                    history[MAX_HISTORY-1] = instruction;
                }

            }

   
}

// Function that returns the instruction from the history
char* returninstruction(int x)
{
    char *tempstring = malloc((strlen(history[x]))*sizeof(char));
    strcpy(tempstring,history[x]);
    return tempstring;

}
  
// Function where the piped system commands is executed
void execArgsPiped(char** parsed, char** parsedpipe)
{
    // 0 is read end, 1 is write end
    int pipefd[2]; 
    pid_t p1, p2;
  
    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }
  
    if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
  
        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        // Parent executing
        p2 = fork();
  
        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }
  
        // Child 2 executing..
        // It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        } else {
            // parent executing, waiting for two children
            wait(NULL);
            wait(NULL);
        }
    }
}
  

  
// function for finding pipe
int parsePipe(char* str, char** strpiped)
{
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }
  
    if (strpiped[1] == NULL)
        return 0; // returns zero if no pipe is found.
    else {
        return 1;
    }
}
  void openHelp()
{
    puts("\n***WELCOME TO MY SHELL HELP***"
        "\nCopyright @ Suprotik Dey"
        "\n-Use the shell at your own risk..."
        "\nList of Commands supported:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>all other general commands available in UNIX shell"
        "\n>pipe handling"
        "\n>improper space handling");
  
    return;
}
// function for parsing command words
void parseSpace(char* str, char** parsed)
{
    int i;
  
    for (i = 0; i < MAX_LIST; i++) {
        parsed[i] = strsep(&str, " ");
  
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}
  // Function to execute builtin commands
int ownCmdHandler(char** parsed)
{
    int NoOfOwnCmds = 4, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
  
    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }
  
    switch (switchOwnArg) {
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        chdir(parsed[1]);
        return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
        username = getenv("USER");
        printf("\nHello %s.\nMind that this is "
            "not a place to play around."
            "\nUse help to know more..\n",
            username);
        return 1;
    default:
        break;
    }
  
    return 0;
}

int processString(char* str, char** parsed, char** parsedpipe)
{
  
    char* strpiped[2];
    int piped = 0;
  
    piped = parsePipe(str, strpiped);
  
    if (piped) {
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);
  
    } else {
  
        parseSpace(str, parsed);
    }
  
    if (ownCmdHandler(parsed))
        return 0;
    else
        return 1 + piped;
}
  
