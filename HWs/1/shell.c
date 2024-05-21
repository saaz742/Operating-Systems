#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80
#define FILE_SEPARATOR "/"
#define REDIRECT_IN "<"
#define REDIRECT_OUT ">"
#define BACKGROUND_OPERATOR "&"

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

tok_t *get_paths()
{
  static tok_t *paths = NULL;
  if (paths == NULL)
  {
    char *env_path = getenv("PATH");
    paths = getToks(env_path);
  }
  return paths;
}

char *combine_path(char *path, char *file)
{
  char *combined_path = malloc(strlen(path) + strlen(file) + 2);
  strcat(combined_path, path);
  strcat(combined_path, FILE_SEPARATOR);
  strcat(combined_path, file);
  return combined_path;
}

int file_exists(char *path)
{
  return access(path, F_OK) == 0;
}

int cmd_quit(tok_t arg[])
{
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);

int cmd_pwd(tok_t arg[])
{
  char *cwd = getcwd(NULL, 0);
  printf("%s\n", cwd);
  free(cwd);
  return 1;
}

int cmd_cd(tok_t arg[])
{
  if (chdir(arg[0]) != 0)
  {
    printf("%s\n", strerror(errno));
  }
  return 1;
}

int cmd_wait(tok_t arg[])
{
  wait(NULL);
  return 1;
}

/* Command Lookup table */
typedef int cmd_fun_t(tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc
{
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_quit, "quit", "quit the command shell"},
    {cmd_pwd, "pwd", "print working directory"},
    {cmd_cd, "cd", "change directory"},
    {cmd_wait, "wait", "wait for background processes"},
};

int cmd_help(tok_t arg[])
{
  int i;
  for (i = 0; i < (sizeof(cmd_table) / sizeof(fun_desc_t)); i++)
  {
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

char *find_program(char *name)
{
  tok_t *paths = get_paths();
  for (int i = 0; i < MAXTOKS && paths[i] != NULL; i++)
  {
    char *path = combine_path(paths[i], name);
    if (file_exists(path))
    {
      return path;
    }
    free(path);
  }
  return NULL;
}

int validate_program(tok_t arg[])
{
  char *path;
  if (strstr(arg[0], FILE_SEPARATOR))
  {
    if (!file_exists(arg[0]))
    {
      printf("%s: command not found\n", arg[0]);
      return FALSE;
    }
  }
  else
  {
    path = find_program(arg[0]);
    if (path == NULL)
    {
      printf("%s: command not found\n", arg[0]);
      return FALSE;
    }
    else
    {
      arg[0] = path;
    }
  }
  return TRUE;
}

int lookup(char cmd[])
{
  int i;
  for (i = 0; i < (sizeof(cmd_table) / sizeof(fun_desc_t)); i++)
  {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  }
  return -1;
}

void init_process(process *p)
{
  p->stdin = STDIN_FILENO;
  p->stdout = STDOUT_FILENO;
  p->stderr = STDERR_FILENO;
  p->background = FALSE;
  p->argv = NULL;
  p->argc = 0;
  p->pid = 0;
  p->completed = FALSE;
  p->stopped = FALSE;
  p->status = 0;
  p->next = NULL;
  p->tmodes = shell_tmodes;
}

__sighandler_t shell_signal_handler_factory(int signum)
{
  return SIG_IGN;
}

__sighandler_t subprocess_signal_handler_factory(int signum)
{
  return SIG_DFL;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive)
  {

    /* force into foreground */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if (setpgid(shell_pgid, shell_pgid) < 0)
    {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
  set_signals(shell_signal_handler_factory);
  first_process = malloc(sizeof(process));
  init_process(first_process);
  first_process->pid = getpid();
}

/**
 * Add a process to our process list
 */
void add_process(process *p)
{
  process *cur = first_process;
  while (cur->next != NULL)
  {
    cur = cur->next;
  }
  cur->next = p;
}

int find_process_io(tok_t *t, char *symbol, int defualt, int flag)
{
  int index = isDirectTok(t, symbol);
  if (index == 0)
  {
    return defualt;
  }
  int file_no = open(t[index + 1], flag);
  if (file_no == -1)
  {
    printf("%s\n", strerror(errno));
    return -1;
  }
  removeTok(t, index, 2);
  return file_no;
}

char is_background(tok_t *t)
{
  int index = isDirectTok(t, BACKGROUND_OPERATOR);
  if (index == 0)
  {
    return FALSE;
  }
  removeTok(t, index, 1);
  return TRUE;
}

/**
 * Creates a process given the inputString from stdin
 */
process *create_process(tok_t *t)
{
  /** YOUR CODE HERE */
  process *p = malloc(sizeof(process));
  init_process(p);
  int stdin_no = find_process_io(t, REDIRECT_IN, STDIN_FILENO, O_RDONLY);
  if (stdin_no == -1)
  {
    return NULL;
  }
  p->stdin = stdin_no;
  int out_flag = O_WRONLY | O_CREAT | O_TRUNC;
  int stdout_no = find_process_io(t, REDIRECT_OUT, STDOUT_FILENO, out_flag);
  if (stdout_no == -1)
  {
    return NULL;
  }
  p->stdout = stdout_no;
  p->stderr = STDERR_FILENO;
  p->background = is_background(t);
  p->argv = t;
  p->argc = countToks(t);
  return p;
}

void run_program(process *p)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    // child
    set_signals(subprocess_signal_handler_factory);
    launch_process(p);
  }
  else
  {
    // parent
    p->pid = pid;
    setpgid(pid, pid);
    if (p->background)
    {
      put_process_in_background(p, FALSE);
    }
    else
    {
      put_process_in_foreground(p, FALSE);
    }
  }
}

int is_whitespace(char *s)
{
  if (s == NULL || *s == '\0')
  {
    return TRUE;
  }
  while (*s != '\0')
  {
    if (!isspace(*s))
    {
      return FALSE;
    }
    s++;
  }
  return TRUE;
}

int shell(int argc, char *argv[])
{
  char *s = malloc(INPUT_STRING_SIZE + 1); /* user input string */
  tok_t *t;                                /* tokens parsed from input */
  int fundex = -1;
  // pid_t pid = getpid();   /* get current processes PID */
  // pid_t ppid = getppid(); /* get parents PID */
  // pid_t cpid, tcpid, cpgid;

  init_shell();

  while (/*printf("\n$ ") &&*/ (s = freadln(stdin)))
  {
    if (is_whitespace(s))
    {
      continue;
    }
    t = getToks(s);        /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */
    if (fundex >= 0)
    {
      cmd_table[fundex].fun(&t[1]);
    }
    else
    {
      process *p;
      if (!validate_program(t) || (p = create_process(t)) == NULL)
      {
        continue;
      }
      add_process(p);
      run_program(p);
    }
  }

  return 0;
}
