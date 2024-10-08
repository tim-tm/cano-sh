#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "cano_sh.h"

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[canosh]$ "

static __attribute__((nonnull))
void execute_command(char **args) {
	int status;
	int pid = fork();
	if(pid < 0) { // error
		fprintf(stderr, "error %s", strerror(errno));
		return;
	} else if(!pid) { // child process
		if(execvp(args[0], args) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
		}
		exit(1);
	} else { // parent process
		int wpid = waitpid(pid, &status, 0);
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
		(void)wpid;
	}	
}
	
// TODO: implement \e \E
// Also hex and octal (and unicode?)
static const char *get_escape_c(char *str, size_t index) {
	if(str[index] == '\\') {
		switch(str[index+1]) {
			case 'a': return "\a";
			case 'b': return "\b";
			case 'c': return "s";
			case 'e': return "e";
			case 'E': return "E";
			case 'f': return "\f";
			case 'n': return "\n";
			case 'r': return "\r";
			case 't': return "\t";
			case 'v': return "\v";
			case '\\': return "\\";
			case '0': return "\0";
			default:
				break;
		}
	}
	return " ";
}

__attribute__((nonnull))
void handle_command(char **args) {
	if(*args == NULL) {
		fprintf(stderr, "error, no command\n");
		return;
	}
	if(strcmp(args[0], "exit") == 0) {
		printf("exit\n");
		int exit_code = 0;
		if(args[1] != NULL) {
			for(size_t i = 0; args[1][i] != '\0'; i++) {
				if(!isdigit(args[1][i])) {
					fprintf(stderr, "numeric argument required - %s\n", args[1]);		
					return;
				}
			}
			exit_code = strtol(args[1], NULL, 10);
		}
		exit(exit_code);
	} else if(strcmp(args[0], "cd") == 0) {
		char const *dir = "~";
		if(args[1] != NULL) {
			dir = args[1];
		}
		if(chdir(dir) < 0) {
			fprintf(stderr, "%s %s", dir, strerror(errno));
		}
	} else if(strcmp(args[0], "echo") == 0) {
		bool newline = true;
		bool escapes = false;
		for(size_t i = 1; args[i] != NULL; i++) {
			for(size_t j = 0; args[i][j] != '\0'; j++) {
				if(args[i][j] == '\\') {
					const char *esc = get_escape_c(args[i], j);
					if(esc[0] == ' ') continue;	
					if(esc[0] == 's') return;
					if(escapes) {
						write(STDOUT_FILENO, esc, 1);
						j++;
					}
				} else {
					write(STDOUT_FILENO, args[i]+j, 1);	
				}
			}
			write(STDOUT_FILENO, " ", 1);
		}
		if(newline)
			write(STDOUT_FILENO, "\n", 1);
	} else if(strcmp(args[0], "history") == 0) {
		//for(size_t i = 0; i < repl->command_his.count; i++) {
			//String command = repl->command_his.data[i];
			//printf("%zu %.*s", i, (int)command.count, command.data);
		//}
	} else {
		execute_command(args);
	}
}

	
char *str_to_cstr(String str) {
	char *cstr = malloc(sizeof(char)*str.count+1);
	if (cstr == NULL)
		return NULL;
	memcpy(cstr, str.data, sizeof(char)*str.count);
	cstr[str.count] = '\0';
	return cstr;
}

char **parse_command(char *command) {
	char const *marker = &command[strlen(command) + 1];
	char *cur = strtok(command, " ");
	if(cur == NULL) {
		return NULL;
	}
	size_t args_s = 8;
	char **args = malloc(sizeof(char*)*args_s);
    if (args == NULL) {
        return NULL;
    }
	size_t args_cur = 0;
	char **resized;
	while(cur != NULL) {
		if(args_cur+2 >= args_s) {
			args_s *= 2;
			resized = realloc(args, sizeof(char*)*args_s);
			if (resized == NULL) {
				free(args);
				return false;
			}
			args = resized;
		}
		while(command[0] != '\0') command++;
		command++;
		assert(command);
		if(marker < command && command[0] == '\'') {
			command++;
			args[args_cur++] = cur;
			cur = command;
			args[args_cur++] = cur;
			while(command[0] != '\'' && command[0] != '\0') command++;
			if(command[0] == '\0') break;
			command[0] = '\0';
			command++;
			cur = strtok(command, " ");
			continue;
		}
		
		args[args_cur++] = cur;
		cur = strtok(NULL, " ");
	}

	args[args_cur] = NULL;

	return args;
}

#ifndef HOT_RELEAD
int main(void) {
	setlocale(LC_ALL, "");
	return shell_repl_run();
}
#endif
