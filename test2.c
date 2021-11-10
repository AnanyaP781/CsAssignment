#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#include<ctype.h>
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define MAX_HISTORY 10

//global variables
char * history[MAX_HISTORY];
int currenthistory = -1, numarg=0;
char bls = 0, ready = 0, running = 1;
char **cmdpnt;
//functions
void historyfeature(char *inst,int num);
//Initial message
void initshell()
{
    clearenv();
    printf("--------------------\n");
    printf("Welcome to my shell\n");
    printf("--------------------\n");
}
// To read the input from the user
char *lsh_read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us

  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We recieved an EOF
    } else  {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

//To parse the command
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}


//to fork the child process 
int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int lsh_execute(char *arg,char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

// Function that returns the instruction from the history
char* returninstruction(int x)
{
    char *tempstring = malloc((strlen(history[x]))*sizeof(char));
    strcpy(tempstring,history[x]);
    return tempstring;

}
// Function Responsible for the shell events
char ** executeshell(char * instruction)
{    char *tempstring = malloc((strlen(instruction))*sizeof(char));
    strcpy(tempstring,instruction);
   
    char ** commands  = lsh_split_line(tempstring);
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
            cmdpnt = lsh_split_line(temp);   
            lsh_execute(cmdpnt[0],cmdpnt);
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
                cmdpnt = lsh_split_line(temp);
                lsh_execute(cmdpnt[0],cmdpnt);
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



//main function
int main(int argc, char **argv)
{
  // Load config files, if any.
char *line;
  char **argument;
  //int should_run=1;
  char **commands;
  // Run command loop.
 initshell();

  while(running) {
    printf("> ");
    line = lsh_read_line();
    if(line[0]!='\n')
    {
      line = strtok(line,"\n\r");
      commands  = executeshell(line);
      if (ready)
      {
        lsh_execute(argument[0],argument);
        ready = 0;
        free(argument); 
      }
      if(bls)
            free(cmdpnt),bls=0;
        
        numarg = 0;
    }
    
   

    free(line);
    free(argument);
    return 0;
  } 
  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}