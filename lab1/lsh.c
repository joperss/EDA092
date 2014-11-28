/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */

//TODO något med foreground tror jag
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"
#define READ (0)
#define WRITE (1)
/*
 * Function declarations
 */

void handle_sigchld(int);
int Execute(int, Command *, Pgm *);
void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

/* When non-zero, this global means the user is done using this program. */
int done = 0;



/* Reap defunct child processes when SIGCHLD is raised. */
void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  /* Ignore SIGINT for the main process */
  struct sigaction sigint;
  sigint.sa_handler = SIG_IGN;
  sigemptyset(&sigint.sa_mask);
  sigint.sa_flags = 0;
  if (sigaction(SIGINT, &sigint, 0) == -1) {
    perror(0);
    exit(1);
  }
  /* Register a sighandler for handling background processes */
  struct sigaction sigchld;
  sigchld.sa_handler = &handle_sigchld;
  sigemptyset(&sigchld.sa_mask);
  sigchld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sigchld, 0) == -1) {
    perror(0);
    exit(1);
  }
  Command cmd;
  int n;

  while (!done) {
    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        PrintCommand(n, &cmd);
        if(n != 1) {
          fprintf(stderr, "Parse failed \n");
        }
        else {
          Execute(1, &cmd, (&cmd)->pgm);
        }  
      }
    }
    
    if(line) {
      free(line);
    }
  }
  return 0;
}

int
Execute (int start, Command *cmd, Pgm *p)
{
  int pifd[2];
  /*create a pipe*/
   if (pipe(pifd) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  char **pl = p->pgmlist;
  /* Checks if the command is exit */
  if (strcmp(pl[0], "exit") == 0) {
    exit(EXIT_SUCCESS);
  }
  /* Checks if the command is cd */
  else if (strcmp(pl[0], "cd") == 0) {
    char *path;
    if (pl[2] != NULL) {
      fprintf(stderr, "Too many arguments\n");
      return 1;
    }
    /* Move to $HOME if no arguments or ~ */
    else if (pl[1] == NULL || !strcmp(pl[1], "~")) {
      path = getenv("HOME");
      if (chdir(path) == -1) {
        fprintf(stderr, "chdir() failed\n");
        return 1;
      }
    }
    /* Else move to the path specified */
    else {
      path = pl[1];
      if (chdir(path) == -1) {
        fprintf(stderr, "chdir() failed\n");
        return 1;
      }
    }
    return 0;
  }
  /* fork a child process */
  pid_t pid = fork();
  if (pid < 0) { /* fork failed */
    fprintf(stderr, "Fork Failed\n");
    return 1;
  }
  else if (pid == 0) { /* child process */
    /* If it is the original call, close the pipe*/
    if (start){
      close(pifd[WRITE]);
      close(pifd[READ]);
    } 
    else { 
      /* The child writes to the parent */
      close(pifd[READ]);
      if (dup2(pifd[WRITE], WRITE) < 0) {
        printf("File descriptor error in child \n");
      }
    }
    if (!cmd->bakground) {
      /* Restore normal SIGINT behaviour if the child process is executed in the foreground */
      struct sigaction sigint;
      sigint.sa_handler = SIG_DFL;
      sigemptyset(&sigint.sa_mask);
      sigint.sa_flags = 0;
      if (sigaction(SIGINT, &sigint, 0) == -1) {
        perror(0);
        exit(EXIT_FAILURE);
      }
    }
    if((cmd->rstdout) != NULL) {
      int rstdout;
      /* Creates/truncates the file specified by cmd->rstdout with the appropriate permissions */
      if(rstdout = open(cmd->rstdout, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) {
        /* Redirect stdout to the file*/
        dup2(rstdout, WRITE); 
        close(rstdout);
      }
      else {
        fprintf(stderr,"Could not write to file: %s \n", cmd->rstdout);
      }
    }
    if(p->next != NULL) {
      Execute(0, cmd, p->next);
    }
    else {
      /* Redirect iput from stdin when appropriate */
      if((cmd->rstdin) != NULL) {
        int rstdin;
        if(rstdin = open(cmd->rstdin, O_RDONLY)) {
           dup2(rstdin, READ);
           close(rstdin);
        }
        else {
          fprintf(stderr,"Failed to open file: %s \n", cmd->rstdin);
        }
      }
    }
    execvp(pl[0], pl);
    exit(EXIT_FAILURE);
  }
  else { /* parent process */
    /* If it is the original call to execute, close the pipe*/
    if (start) {
      close(pifd[WRITE]);
      close(pifd[READ]);
    } 
    else { /* The parent reads from the child */
      close(pifd[WRITE]);
      if (dup2(pifd[READ], READ) < 0) {
        printf("File descriptor error in parent \n");
      }
    }
    /* Don't wait for background processes */
    if (cmd->bakground) ;
    else { 
      int status;
      waitpid(pid, &status, 0);
      
      /* Checks the exit status of the child process */
      if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGINT) {
          printf("\nKeyboard interrupt\n");
        }
        else {
          printf("\n");
        }
      }
      else if (status) {
        fprintf(stderr, "Invalid command\n");
      }
    }
  }
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (whitespace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && whitespace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}
