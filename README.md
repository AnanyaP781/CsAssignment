# CsAssignment
This project comprises of writing a code for a simple UNIX shell with the following functionalities.
- Creating a child process and executing commands using the execvp() function
- Adding a history feature ; '!!' command displays the previous command
- Supports < and > redirection
- Allows communication between two processes via a pipe using the command '|'

Code reference [here](https://github.com/nhutnamhcmus/simple-shell)
 
 ## Flow of the program
 - **int main()** initially runs the **initshell()** which displays the opening message.
 - the input is read by the function **read_input()**.
 - the input is then parsed by the **parse_command()** function which also checks for the presence of '&' and the arguments are returned to the main function.
 - if the input command is '!!' the path is redirected to the history feature else the **set_prev_command()** saves the input to the history and **exec_command()** executes the parsed     input.
 - **exec_command()** checks whether the command has an input or output redirect using **redirect()**. **redirect()** uses the **is_redirect()** function to check for the presence of the < or > operator and if present is parsed by **parse_redirect**. If a redirect is present they are executed by the functions **exec_child_inputred()** or **exec_child_outputred()** respectively which in turn pass the arguments to **exec_child()** which pas it to the execvp() function for execution.
 - **simple_shell_pipe()** checks the presence of pipes in the input by passing the arguments to**is_pipe()** function. If the input is a piped command it is parsed by **parse_pipe()** and executed by the **exec_child_pipe()** function.
 - Incase the input command is a builtin command "cd", "help" or "exit", the functions **simple_shell_cd**, **simple_shell_help** or **simple_shell_exit** are invoked respectively.
