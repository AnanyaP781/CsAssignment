#include <stdio.h>  // printf(), fgets()
#include <string.h> // strtok(), strcmp(), strdup()
#include <stdlib.h> // free()
#include <unistd.h> // fork()
#include <sys/types.h>
#include <sys/wait.h> // waitpid()
#include <sys/stat.h>
#include <fcntl.h> // open(), creat(), close()
#include <time.h>
#include <errno.h>

#define MAX_LINE_LENGTH 1024
#define BUFFER_SIZE 64
#define REDIR_SIZE 2
#define PIPE_SIZE 3
#define MAX_HISTORY_SIZE 128
#define MAX_COMMAND_NAME_LENGTH 128

#define PROMPT_FORMAT "%F %T "
#define PROMPT_MAX_LENGTH 30

#define TOFILE ">"
#define FROMFILE "<"
#define PIPE_OPT "|"

int running = 1;

int simple_shell_cd(char **args);
int simple_shell_help(char **args);
int simple_shell_exit(char **args);
void exec_command(char **args, char **redir_argv, int wait, int res);

//Opening message
void init_shell()
{
    clearenv();
    printf("--------------------\n");
    printf("Welcome to my shell\n");
    printf("--------------------\n");
}

//replaces the end of the string with '\0'
void remove_end_of_line(char *line) {
    int i = 0;
    while (line[i] != '\n') {
        i++;
    }

    line[i] = '\0';
}

//Funtion to read the input
//Does not return any value
void read_line(char *line) 
{
    char *ret = fgets(line, MAX_LINE_LENGTH, stdin);

    remove_end_of_line(line);

    //To end the program on encountering EXIT or QUIT or NULL
    if (strcmp(line, "exit") == 0 || ret == NULL || strcmp(line, "quit") == 0 || strcmp(line, "EXIT") == 0 || strcmp(line, "QUIT") == 0) 
    {
        exit(EXIT_SUCCESS);
    }
}

//Parses the input commad into arguments
//int *wait checks for the presence of & (parent and child will run concurrently)
void parse_command(char *input_string, char **argv, int *wait) 
{
    int i = 0;

    while (i < BUFFER_SIZE) 
    {
        argv[i] = NULL;
        i++;
    }

    //if & is present wait = 0 else wait = 1
    *wait = (input_string[strlen(input_string) - 1] == '&') ? 0 : 1; 
    input_string[strlen(input_string) - 1] = (*wait == 0) ? input_string[strlen(input_string) - 1] = '\0' : input_string[strlen(input_string) - 1];
    i = 0;
    argv[i] = strtok(input_string, " ");

    if (argv[i] == NULL) 
        return;

    while (argv[i] != NULL) 
    {
        i++;
        argv[i] = strtok(NULL, " ");
    }

    argv[i] = NULL;
}

//checks for I/o redirection i.e < or >
//Returns the position of < or > is present else returns zero
int is_redirect(char **argv) 
{
    int i = 0;
    while (argv[i] != NULL) 
    {
        if (strcmp(argv[i], TOFILE) == 0 || strcmp(argv[i], FROMFILE) == 0) 
        {
            return i; 
        }
        i = -~i; 
    }
    return 0; 
}

//Checks if pipes are present
//Returns the position of the pipe else returns 0 if the pipe is absent
int is_pipe(char **argv) 
{
    int i = 0;
    while (argv[i] != NULL) 
    {
        if (strcmp(argv[i], PIPE_OPT) == 0) 
        {
            return i;
        }
        i = -~i; 
    }
    return 0; 
}
void parse_redirect(char **argv, char **redirect_argv, int redirect_index)
{
    redirect_argv[0] = strdup(argv[redirect_index]);
    redirect_argv[1] = strdup(argv[redirect_index + 1]);
    argv[redirect_index] = NULL;
    argv[redirect_index + 1] = NULL;
}


//EXECUTION 

//Executes a piped command
void parse_pipe(char **argv, char **child1_argv, char **child2_argv, int pipe_index)
{
    int i = 0;
    for (i = 0; i < pipe_index; i++)
    {
        child1_argv[i] = strdup(argv[i]);
    }
    child1_argv[i++] = NULL;

    while (argv[i] != NULL)
    {
        child2_argv[i - pipe_index - 1] = strdup(argv[i]);
        i++;
    }
    child2_argv[i - pipe_index - 1] = NULL;
}

//Function to execute the child function
//Passes the arguments to execvp()
void exec_child(char **argv) 
{
    if (execvp(argv[0], argv) < 0) 
    {
        fprintf(stderr, "Error: Failed to execte command.\n");
        exit(EXIT_FAILURE);
    }
}

//Onput redirection
void exec_child_inputred(char **argv, char **dir) 
{
    // osh>ls < out.txt

    int fd_in;
    fd_in = open(dir[1], O_RDONLY);
    if (fd_in == -1) 
    {
        perror("Error: Redirect input failed");
        exit(EXIT_FAILURE);
    }

    dup2(fd_in, STDIN_FILENO);

    if (close(fd_in) == -1) 
    {
        perror("Error: Closing input failed");
        exit(EXIT_FAILURE);
    }
    exec_child(argv);
}

//Output redirection
void exec_child_outputred(char **argv, char **dir) 
{
    // osh>ls > out.txt

    int fd_out;
    fd_out = creat(dir[1], S_IRWXU);
    if (fd_out == -1) 
    {
        perror("Error: Redirect output failed");
        exit(EXIT_FAILURE);
    }
    dup2(fd_out, STDOUT_FILENO);

    if (close(fd_out) == -1) 
    {
        perror("Error: Closing output failed");
        exit(EXIT_FAILURE);
    }

    exec_child(argv);
}

//Executes piped commands
void exec_child_pipe(char **argv_in, char **argv_out) 
{
    int fd[2];
    // p[0]: read end
    // p[1]: write end
    if (pipe(fd) == -1) 
    {
        perror("Error: Pipe failed");
        exit(EXIT_FAILURE);
    }

    //child 1 exec input from main process
    //write to child 2
    if (fork() == 0) 
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_child(argv_in);
        exit(EXIT_SUCCESS);
    }

    //child 2 exec output from child 1
    //read from child 1
    if (fork() == 0) 
    {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        exec_child(argv_out);
        exit(EXIT_SUCCESS);
    }

    close(fd[0]);
    close(fd[1]);
    wait(0);
    wait(0);    
}

//HISTORY

void set_prev_command(char *history, char *line) 
{
    strcpy(history, line);
}

char *get_prev_command(char *history) 
{
    if (history[0] == '\0') {
        fprintf(stderr, "No commands in history\n");
        return NULL;
    }
    return history;
}

int simple_shell_history(char *history, char **redir_args) {
    char *cur_args[BUFFER_SIZE];
    char cur_command[MAX_LINE_LENGTH];
    int t_wait;

    if (history[0] == '\0') {
        fprintf(stderr, "No commands in history\n");
        return 1;
    }
    strcpy(cur_command, history);
    printf("%s\n", cur_command);
    parse_command(cur_command, cur_args, &t_wait);
    int res = 0;
    exec_command(cur_args, redir_args, t_wait, res);
    return res;
}

int simple_shell_redirect(char **args, char **redir_argv) 
{
    // printf("%s", "Executing redirect\n");
    int redir_op_index = is_redirect(args);
    // printf("%d", redir_op_index);
    if (redir_op_index != 0) 
    {
        parse_redirect(args, redir_argv, redir_op_index);
        if (strcmp(redir_argv[0], ">") == 0) 
        {
            exec_child_outputred(args, redir_argv);
        } 
        else if (strcmp(redir_argv[0], "<") == 0) 
        {
            exec_child_inputred(args, redir_argv);
        } 
        return 1;
    }
    return 0;
}
//Checks the presence of pipes
//Reurns 1 if pipes are present else returns 0
int simple_shell_pipe(char **args) 
{
    int pipe_op_index = is_pipe(args);
    if (pipe_op_index != 0) 
    {  
        // printf("%s", "Exec Pipe");
        char *child01_arg[PIPE_SIZE];
        char *child02_arg[PIPE_SIZE];   
        parse_pipe(args, child01_arg, child02_arg, pipe_op_index);
        exec_child_pipe(child01_arg, child02_arg);
        return 1;
    }
    return 0;
}

//Built in commands
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};
int (*builtin_func[])(char **) = {
    &simple_shell_cd,
    &simple_shell_help,
    &simple_shell_exit
};
int simple_shell_num_builtins() 
{
    return sizeof(builtin_str) / sizeof(char *);
}

int simple_shell_cd(char **argv) 
{
    if (argv[1] == NULL) 
    {
        fprintf(stderr, "Error: Expected argument to \"cd\"\n");
    } else {
        // Change the process's working directory to PATH.
        if (chdir(argv[1]) != 0) {
            perror("Error: Error when change the process's working directory to PATH.");
        }
    }
    return 1;
}
int simple_shell_help(char **argv) 
{
    static char help_team_information[] =
        "UNIX SHELL\n"
        "191MT006 Ananya P\n"
        "This UNIX shell is a simple C program in POSIX that replicates the basic functions of a shell.\n"
        "This program was written with reference to Khoa and Nam's Shell.\n"
        "\n"
        "\nUsage help command.\n"
        "Options for [command name]:\n"
        "cd <directory name>\t\t\tDescription: Changes the current working directory.\n"
        "exit              \t\t\tDescription: Exit UNIX shell, buyback Linux Shell.\n";
    static char help_cd_command[] = "HELP CD COMMAND\n";
    static char help_exit_command[] = "HELP EXIT COMMAND\n";

    if (strcmp(argv[0], "help") == 0 && argv[1] == NULL) 
    {
        printf("%s", help_team_information);
        return 0;
    }
    if (strcmp(argv[1], "cd") == 0) 
    {
        printf("%s", help_cd_command);
    } 
    else if (strcmp(argv[1], "exit") == 0) 
    {
        printf("%s", help_exit_command);
    } 
    else 
    {
        printf("%s", "Error: Too much arguments.");
        return 1;
    }
    return 0;
}
int simple_shell_exit(char **args) {
    running = 0;
    return running;
}



//Executes simple and built in commands
void exec_command(char **args, char **redir_argv, int wait, int res) 
{
    //Checks for the presence of builtin commands
    for (int i = 0; i < simple_shell_num_builtins(); i++) 
    {
        if (strcmp(args[0], builtin_str[i]) == 0) 
        {
            (*builtin_func[i])(args);
            res = 1;
        }
    }
    
    if (res == 0) 
    {
        int status;

        //Creating a child process using fork()
        pid_t pid = fork();
        if (pid == 0) 
        {
            //Child process
            if (res == 0) 
                res = simple_shell_redirect(args, redir_argv);
            if (res == 0) 
                res = simple_shell_pipe(args);
            if (res == 0) 
                execvp(args[0], args);
            exit(EXIT_SUCCESS);

        } else if (pid < 0)
        { //Failure of child process creation
            perror("Error: Error forking");
            exit(EXIT_FAILURE);
        } else 
        {   // Background process
            // Parent process
            if (wait == 1) 
            {
                waitpid(pid, &status, WUNTRACED); // 
            }
        }
    }
}

//Main function
int main(void) 
{
    
    char *args[BUFFER_SIZE];
    char line[MAX_LINE_LENGTH];
    char t_line[MAX_LINE_LENGTH];
    char history[MAX_LINE_LENGTH] = "No commands in history";
    char *redir_argv[REDIR_SIZE];
    int wait;

    // Opening message
    init_shell();
    int res = 0;

    //Initialize an infinite loop
    while (running) 
    {
        printf("osh>");
        fflush(stdout);

        
        read_line(line);
        strcpy(t_line, line);
        parse_command(line, args, &wait);

        // Thực thi lệnh
        if (strcmp(args[0], "!!") == 0) 
        {
            res = simple_shell_history(history, redir_argv);
        } else 
        {
            set_prev_command(history, t_line);
            exec_command(args, redir_argv, wait, res);
        }
        res = 0;
    }
    return 0;
}