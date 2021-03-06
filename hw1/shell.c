#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);
int cmd_cd(tok_t agr[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
	{cmd_cd, "cd", "change the directory"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int cmd_cd(tok_t arg[]){
	if(chdir(arg[0])==-1){
		printf("cannot find the directory!\n");	
	}	
	return 1;
}
int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){

    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	//signal(SIGTTIN,SIG_IGN);
	//signal(SIGTTOU,SIG_IGN);
	//signal(SIGCHLD,SIG_IGN);
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
  /** YOUR CODE HERE */
  return NULL;
}

void shift_string(char *str[],int i){
	while(str[i]){
		str[i] = str[i+1];
		i++;
	}
}

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
	char concat_s[1024];
	char cwd[1024];
	char *path;
  tok_t *t;			/* tokens parsed from input */
	tok_t *path_token;
  int lineNum = 0;
  int fundex = -1;
	int status;
	int i;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;
	struct stat file_stat;
	first_process = NULL;
	
  init_shell();

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

  lineNum=0;
  fprintf(stdout, "%d: ", lineNum++);
	if(getcwd(cwd,sizeof(cwd))){
		fprintf(stdout,"%s ",cwd);
	}
  while ((s = freadln(stdin))){
		process* process_ptr = fisrt_process;
		if(process_ptr){
			while(process_ptr->next){
				process_ptr = process_ptr->next;
			}
			process_ptr->next = (process*)malloc(sizeof(process));
			process_ptr = process_ptr->next;
		}
		else{
			first_process = (process*)malloc(sizeof(process));
			process_ptr = first_process;
		}

    t = getToks(s); /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */
    if(fundex >= 0) cmd_table[fundex].fun(&t[1]);
    else {
			path = getenv("PATH");
  		cpid = fork();
			if(cpid == 0){
				i=0;
				while(t[i]){
					if(t[i][0] == '<'){
						int in = open(t[i+1],O_RDONLY);
						process_ptr->stdin = in;
						if(in < 0){
							fprintf(stdin,"no %s file\n",t[i+1]);
							exit(EXIT_FAILURE);
						}
						dup2(in,0);
						close(in);
						t[i] = NULL;
						break;
					}
					else if(t[i][0] == '>'){
						int out = open(t[i+1],O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
						dup2(out,1);
						close(out);
						t[i] = NULL;
						break;
					}
					i++;
				}
				path_token = getToks(path);
				i=0;
				while(path_token[i]){
					strcpy(concat_s,path_token[i]);
					strcat(concat_s,"/");
					strcat(concat_s,t[0]);
					if(stat(concat_s,&file_stat) == 0){
						execv(concat_s,&t[0]);
						perror("Error");
						exit(1);
					}
					i++;
				}
				execv(t[0],&t[0]);
				perror("Error");
				exit(1);
			}
			else{
				tcpid = wait(&status);	
				if(tcpid != cpid){
					perror("Error occurs in wait process\n");
				}
			}
		}
		
    fprintf(stdout, "%d: ", lineNum++);
		if(getcwd(cwd,sizeof(cwd))){
			fprintf(stdout,"%s ",cwd);
		}
  }
  return 0;
}
