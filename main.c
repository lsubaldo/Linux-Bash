#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h> 
#include <stdbool.h> 

typedef struct node {
	pid_t p;
	struct node * next;
} node;

typedef struct directory {
	char dir[32];
	struct directory *next; 
} directory; 

/* main helper functions */

char** parse(const char *s, char* split) {
	char* token; 
	char* count_cpy = strdup(s);
	char* tok_cpy = strdup(s);

	int count = 0;
	for (token = strtok(count_cpy, split); token != NULL; 
		token = strtok(NULL, split)) { 
		count++;
	} 

	int size = sizeof(char*);
	char** tokens = malloc((count+1) * size);

	int i = 0;
	for (token = strtok(tok_cpy, split); token != NULL; 
		token = strtok(NULL, split)) { 
		tokens[i] = strdup(token);
		i++;
	}
	tokens[count] = NULL;
	free(count_cpy);
	free(tok_cpy);
	return tokens;
}

void print_tokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}

void uncomment(char* str) {
	//Takes care of comments
	int num = 0;
	while (num < strlen(str)) {
		if (str[num] == '#') {
			str[num] = '\0'; 
			break; 
		}
		num++; 
	}
}

void check() {
    int stat;
	waitpid(-1, &stat, WNOHANG);
}

directory *load_dir(const char *filename, int *num) {
	FILE *fp = fopen(filename, "r"); 
	directory *head = NULL; 
	char new[256];
	while (fgets(new, 256, fp) != NULL) {
		char *new_dir = strtok(new, " \t\n");
		if (head == NULL)  {
			directory *d = NULL;
			strcpy(d->dir, new_dir);
			d->next = NULL; 
			head = d;	
		}
		else {
			directory *tmp = NULL;
			strcpy(tmp->dir, new_dir);
			tmp->next = head;
			head = tmp;
		} 
		(*num)++; //keeps track of the number of directories there are 
					//is this even necessary? 
	}
	fclose(fp);
	return head;
}


bool is_file(directory *dir_list, char *buf) {
	struct stat statresult; 
	bool is_f = false;
	while (dir_list != NULL) {
		char *command = strcat(dir_list->dir, "/");
		command = strcat(command, buf); 
		int rv = stat(command, &statresult); 
		if (rv == 0) {
			is_f = true; 
			break;
		}
		dir_list = dir_list->next; 
	}
	return is_f; 
}

/* MAIN */ 
int main(int argc, char **argv) {

	int num_dir = 0; 
	directory *shell_dir = load_dir("shell-config.txt", &num_dir); 
	// mode settings
	//sequential = 0;
	//parallel = 1; 
	int mode = 0;
	int change = 0;
	char *mode_arr[] = {"sequential", "parallel"};

    char buffer[1024];
	while (1) { // loop only exits through an exit() system call
		if (mode == 1) {
        	check();
			signal(SIGCHLD, check);
		}

		/* Code for Part II 
        struct pollfd pfd[1];
		pfd[0].fd = 0;
		pfd[0].events = POLLIN;
        pfd[0].revents = 0;
        poll(&pfd[0], 1, 1000);
		*/

		// command prompt loop
		char * prompt = "Prompt> ";
		printf("%s", prompt);
		fflush(stdout); //prints immediately 
		
		fgets(buffer, 1024, stdin);
		uncomment(buffer);
		
		/*parse input --> array of pointers filled with arrays of 
		pointers */
		char** command_chunks = parse(buffer, ";"); //parse by ";"
		
		// while loop to get size of command_chunks
		int chunk_count = 0;
		while (command_chunks[chunk_count] != NULL) {
			chunk_count++;
		}
		char** command_list[chunk_count+1]; 
    	char* whitespace = " \t\r\n";
		for (int i = 0; i < chunk_count; i++) {
			command_list[i] = parse(command_chunks[i], whitespace);
		}
		command_list[chunk_count] = NULL;
		char *ex = "exit"; 
		char *mode_word = "mode";
		char *seq = "s";
		char *par = "p";
		if (mode == 0) { // sequential mode
			int i = 0;
	  	    while (command_list[i] != NULL) {
				if (command_list[i][0] == NULL) { // user enters only spaces, e.g. ; " " ; or " "
					i++;
				}	
				else {			
					int j = 0;
  					if (strcasecmp(command_list[i][j], ex) == 0) {
			    		printf("Exit called. Goodbye.\n");
			    		fflush(stdout);
			    		exit(0);      //exit takes a parameter 
					}
					else if (strcasecmp(command_list[i][j], mode_word) == 0) {
		   	    		j++;
						//printf("%s\n", command_list[i][j]); 
			    		if (command_list[i][j] == NULL) {
							printf("Current mode: %s\n", mode_arr[mode]);
			    		}
			    		else if (strcasecmp(command_list[i][j], seq) == 0) {
							change = 0;
			    		}
			    		else if (strcasecmp(command_list[i][j], par) == 0) {
							change = 1;
			    		}
					}
					else {
						if (is_file(shell_dir, command_list[i][j])) { 
							pid_t p = fork(); 
							if (p == 0) {
								if (execv(command_list[i][j], command_list[i]) < 0) {
					       			fprintf(stderr, "execv failed: %s\n", strerror(errno));
								}
							}
							else if (p > 0) {
								wait(&p);
							}
						}
					} //end of else
					i++;
				} //end of outer else 
			} //end of while loop
		} //end of sequential 

		else if (mode == 1) { //parallel mode
			int i = 0;
			node *list = NULL;
			while (command_list[i] != NULL) {
				if (command_list[i][0] == NULL) { //user enters only spaces, e.g. ; " " ; or " "
					i++;
				}
				else {			
					int j = 0;
  					if (strcasecmp(command_list[i][j], ex) == 0) { //exit command called
			    		printf("Exit called. Goodbye.\n");
			    		fflush(stdout);
			    		exit(0);      
					} //end of if statement
					else if (strcasecmp(command_list[i][j], mode_word) == 0) { //mode
		   	    		j++;
						//printf("%s\n", command_list[i][j]); 
			    		if (command_list[i][j] == NULL) { // print mode
							printf("Current mode: %s\n", mode_arr[mode]);
			    		}
			    		else if (strcasecmp(command_list[i][j], seq) == 0) { // change mode (sequential)
							change = 0;
			    		}
			    		else if (strcasecmp(command_list[i][j], par) == 0) { // change mode (parallel)
							change = 1;
			    		}
					} //end of else if
					else {
						pid_t p = fork(); 
						int ran = 0;
						if (p == 0) {
							setpgid(0, 0);
							if (execv(command_list[i][j], command_list[i]) < 0) {
		           				fprintf(stderr, "execv failed: %s\n", strerror(errno));	
							}
							else {
								ran = 1; 
							}
						}
						else if (p > 0) {
							if (ran == 1) {
								if (list == NULL)  {
									node *head = NULL; 
									head->p = p;
									head->next = NULL; 
									list = head;	
								}
								else {
									node *tmp = NULL;
									tmp->p = p;
									tmp->next = list;
									list = tmp;
								} 
							}
						} //end of parent statement
					} //end of else
					i++;
				} //end of outer else statement
			} //end of parallel while loop
			node * iter_node = list; //in original wait loop
			while (iter_node != NULL) {
				int status = 0; 
				waitpid(-1, &status, WNOHANG);
				iter_node = iter_node->next;
			}
		} //end of parallel mode
		mode = change; // mode changes if 'change' has been modified
	} //end of big while loop
	return 0;
}
