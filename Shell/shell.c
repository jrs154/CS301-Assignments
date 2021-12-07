#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;
typedef struct input_s {
    int input;
    int output;
    int num_args;
    char **args;
} Input_s;


pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_cwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc 
{
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = 
{
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_cd, "cd", "change the current directory to given address"},
    {cmd_cwd, "cwd", "print the current working directory"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

int cmd_cd(struct tokens* tokens)
{
  int arglen = tokens_get_length(tokens);
  //Temporary
  if(arglen==1)
  {
    if(!chdir(getenv("HOME")))
      return 0;
  }

  else if(arglen==2)
  {
    if(!chdir(tokens_get_token(tokens, 1)))
      return 0;
  }

  else
    fprintf(stderr, "More than 2 arguments not accepted\n");
  
  return 0;
}

int cmd_cwd(struct tokens* tokens)
{
  char *path;
  path = getcwd(NULL, 0);
	printf("%s", path);
	printf("\n");
	return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}


/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();
  //signal(SIGINT, sigint_handler);
  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) 
  {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) 
    {
      cmd_table[fundex].fun(tokens);
    } 
    else 
    {
      /* REPLACE this to run commands as programs. */
      int status;
      pid_t pid = fork(); 
      if (pid > 0) 
        wait(&status);
      else
      {
        int count = 1;
        int token_len = tokens_get_length(tokens);
        //char params[token_len][100];
        char **params = (char **) malloc(token_len);
        for(int i=0; i<token_len; i++)
        {
          params[i] = (char *) malloc(1000);
          strcpy(params[i], tokens_get_token(tokens, i));
          if(strcmp(params[i], "|") == 0)
            count += 1;
        }
        if (count==1)
        {
          execvp(tokens_get_token(tokens, 0), params);
          free(params);
        }

        else
        {
          int num = 1;
          int num_tokens = tokens_get_length(tokens);
          for (int i = 0; i < num_tokens; i++) 
          {
            if (strcmp(tokens_get_token(tokens,i), "|") == 0) 
            {
              num += 1;
            }
          }

          
          Input_s* inp = (Input_s *) malloc(num * sizeof(Input_s));
          
          for (int i = 0; i < num; i++) 
          {
            inp[i].input = STDIN_FILENO;
            inp[i].output = STDOUT_FILENO;
            inp[i].num_args = 0;
            inp[i].args = (char **) NULL;
          }

          int j = 0;
          for (int i = 0; i < num_tokens; i++) 
          {
            if (strcmp(tokens_get_token(tokens,i), "|") == 0) 
            {
              j += 1;
            }

            else
            {
              inp[j].num_args += 1;
            }

            
          }
          for (int i = 0; i < num; i++) 
          {
            inp[i].args = (char **) malloc((inp[i].num_args + 1) *sizeof(char *));
            if (inp[i].args == NULL) 
            {
              for (int j = 0; j < i; j++) 
              {
                free(inp[j].args);
              }
              free(inp);
            }
          }
          int k = 0;
          int l = 0;

          for (int i = 0; i < num_tokens; i++) 
          {
                  
            if (strcmp(tokens_get_token(tokens,i), "|") == 0) 
            {
              inp[k].args[l] = (char *) NULL;
              k += 1;
              l = 0;
            }
            
            else 
            {
              char *src;
              size_t len;

              src = tokens_get_token(tokens,i);
              len = strlen(tokens_get_token(tokens,i));
              inp[k].args[l] = (char *)malloc((len + 1) * sizeof(char));
              
              if (inp[k].args[l] == NULL) 
              {
                  for (int m = 0; m < num; m++) 
                  {
                      free(inp[m].args);
                  }
                  free(inp);
              }
                
              strncpy(inp[k].args[l], src, len);
              inp[k].args[l][len] = '\0';
              l += 1;
            } 
          }
          inp[k].args[l] = (char *) NULL;

          num = 1;

          for (int i = 0; i < token_len; i++) 
          {
            if (strcmp(tokens_get_token(tokens,i), "|") == 0) 
            {
              num += 1;
            }
          }

          Input_s *arr = inp;

          pid_t pid;
          int index = 0;
          int fd[2];

          for (index = 0; index < num; index++)
          {
            if (pipe(fd) == -1) 
            {
              exit(1);
            }

            if (index < num - 1) 
            {
              arr[index].output = fd[1];
              arr[index + 1].input = fd[0];
            }

            if ((pid = fork()) < 0) 
            {
              exit(1);
            }

            else if (pid == 0) 
            { // Child process
              close(fd[0]);
              if (arr[index].input != STDIN_FILENO) 
              {
                dup2(arr[index].input, STDIN_FILENO);
                close(arr[index].input);
              }
              if (arr[index].output != STDOUT_FILENO) 
              {
                  dup2(arr[index].output, STDOUT_FILENO);
                  close(arr[index].output);
              }
              execvp(arr[index].args[0], arr[index].args);
              perror(arr[index].args[0]);
              exit(1);
            }

            else 
            { // Parent process
              close(fd[1]);
              wait(&status);
            }
          }
        }
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
