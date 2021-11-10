#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#include<ctype.h>
#define MAX_LINE 80
#define MAX_HISTORY 10
#define MAX_ARG 15

//Global variables
int sizeofcmd, numarg=0, currenthistory=-1;
char running = 1, tempbool=0, bls=0, ready=0;
char * history[MAX_HISTORY];
char **cmdpnt;
//Opening message for the shell
void initshell()
{
    clearenv();
    printf("--------------------\n");
    printf("Welcome to my shell\n");
    printf("--------------------\n");
}
//To display the current directory
void printDir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s", cwd);
}
// Function that returns the instruction from the history
char* returninstruction(int x)
{
    char *tempstring = malloc((strlen(history[x]))*sizeof(char));
    strcpy(tempstring,history[x]);
    return tempstring;

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
//Function that executes the commands using forks
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
// Function for History feature
void historyfeature(char *inst,int num)
{
    int i;
    char *temp;
    if(num==0)
    {
        if(currenthistory!=-1)
        {
            for(i=0;i<currenthistory+1;i++)
              printf("%d: %s \n",i,history[i]);
        }
        else
        {
            printf("No commands in history\n");
        }  
    }
    else if(num==1)
    {
        if(currenthistory!=-1)
        {
            temp = returninstruction(currenthistory);
            numarg = 0;
            cmdpnt = parsetext(temp);   
            execute(cmdpnt[0],cmdpnt);
            free(temp);
            bls=1;
        }
        else printf("No Commands in History 1\n");
           
        }
   
    else if(num==2)        // !digit Functionality
        {
            if(currenthistory !=-1)
            {
                int x =inst[1]-'0';
           
            if(x>=0 && x<=currenthistory)
            {
                temp = returninstruction(x);
                numarg = 0;
                cmdpnt = parsetext(temp);
                execute(cmdpnt[0],cmdpnt);
                free(temp);
                bls=1;
           
            }
            else printf("No Such Commands in History\n");
            }
        else printf("No Commands in History 2\n");
        }

    else if(num==3)    // Saving in history functionality
        {    if(currenthistory!=MAX_HISTORY-1)
            {    currenthistory=currenthistory +1;
                history[currenthistory] = inst;
            }
            else if(currenthistory==MAX_HISTORY-1)
                {    int x;
                    for(x=1;x<MAX_HISTORY;x++)
                        history[x-1] = history[x];
                    history[MAX_HISTORY-1] = inst;
                }

            }


    
}



//Main function
int main(void)
{
    char *args[MAX_LINE/2 -1];
    char incmd[MAX_LINE];
     char *buffer = NULL;
     size_t x;
    int should_run = 1,size = 0;
    initshell();
    printDir();
    //while(should_run)
    {
        printf(">osh");
        fflush(stdout);
        size = getline(&buffer, &x, stdin);
        if(sizeofcmd == -1 && sizeofcmd < MAX_LINE)
        printf("Error Reading the Line \n");

    }
}