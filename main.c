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
	char dir[256];
	struct directory *next; 
} directory; 

/* main helper functions */

/*
 *Parses through the entries and splits it on split (either ";" or " ") 
 */
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

/*
 *Null-terminates comments
 */
void uncomment(char* str) {
	int num = 0;
	while (num < strlen(str)) {
		if (str[num] == '#') {
			str[num] = '\0'; 
			break; 
		}
		num++; 
	}
}

/*
 *Loads the paths from the file, shell-config, into a linked list of type directory. 
 */
directory *load_dir(const char *filename) {
	FILE *fp = fopen(filename, "r"); 
	directory *head = NULL; 
	char new[256];
	while (fgets(new, 256, fp) != NULL) {
		new[strlen(new)-1] = '/'; 
		directory *d = malloc(sizeof(directory));
		strcpy(d->dir, new);
		if (head == NULL)  {
			d->next = NULL; 
			head = d;	
		}
		else {
			directory *tmp = head;
			while (tmp->next != NULL) {
				tmp = tmp->next; 
			}
			d->next = NULL; 
			tmp->next = d; 
		} 
	}
	fclose(fp);
	return head;
}

/*
 *Frees all the entries in the directory linked list 
 */
void free_paths(directory *paths) {
    while (paths != NULL) {
         directory *temp = paths; 
         paths = paths->next;
	 	free(temp);
    }
}

//checks whether command is a valid file in a directory 
char *is_file(directory *dir_list, char *buf) {
	struct stat statresult; 
	while (dir_list != NULL) {
		char *temp = strdup(dir_list->dir); 
		char *command = strcat(temp, buf); 
		int rv = stat(command, &statresult); 
		if (rv == 0) {
			return command;
		}
		free(temp);
		dir_list = dir_list->next; 
	} 
	return NULL;
}

/*
 *Put here for debugging purposes. 
 */
void list_print(const directory *list) {
    int i = 0;
    printf("In list_print\n");
    while (list != NULL) {
        printf("List item %d: %s\n", i++, list->dir);
        list = list->next;
    }
}

/* MAIN */ 
int main(int argc, char **argv) {

	directory *shell_dir = load_dir("shell-config"); 
	//list_print(shell_dir); 

	// mode settings
	//sequential = 0;
	//parallel = 1; 
	int mode = 0;
	int change = 0;
	char *mode_arr[] = {"sequential", "parallel"};

    char buffer[1024];
	while (1) { // loop only exits through an exit() system call
		if (mode == 1) {
        	int stat;
			waitpid(-1, &stat, WNOHANG);
			//signal(SIGCHLD, check);
		}

		/* Code for Part II */
        struct pollfd pfd[1];
		pfd[0].fd = 0;
		pfd[0].events = POLLIN;
        pfd[0].revents = 0;

        int rv = poll(&pfd[0], 1, 1000);
		if (rv == 0) {
			printf("Timeout\n");
		}
		else if (rv > 0) {
			printf("You typed something on stdin\n");
		}
		else {
			printf("There was some kind of error: %s\n", strerror(errno)); 
		}
		
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

		//Built-in function and mode variables 
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
  					if (strcasecmp(command_list[i][j], ex) == 0) { //exit command called
			    		printf("Exit called. Goodbye.\n");
			    		fflush(stdout);
						free_paths(shell_dir);   //free linked list before exit
						free_tokens(command_chunks); //free tokens before exit
			    		exit(0);     
					}
					else if (strcasecmp(command_list[i][j], mode_word) == 0) { //mode
		   	    		j++;
						//print current mode if mode is not specified 
			    		if (command_list[i][j] == NULL) {
							printf("Current mode: %s\n", mode_arr[mode]);
			    		}
						//change mode to sequential
			    		else if (strcasecmp(command_list[i][j], seq) == 0) {
							change = 0;
			    		}
						//change mode to parallel
			    		else if (strcasecmp(command_list[i][j], par) == 0) {
							change = 1;
			    		}
					}
					else {
						char *cmd = NULL;
						bool has_slash = false;
						char *b = command_list[i][j];
						for (int n=0 ; n<strlen(b); n++) {
							if (b[n] == '/') {
								has_slash = true;
								break;
							}
						}
					//if it does, set cmd to b so that it can run as if it was in part 1
						if (has_slash) {
							cmd = b; 
						}
				//if it does not, check to make sure it is part of a valid path for part 2
						else {
							cmd = is_file(shell_dir, command_list[i][j]);
						}
						if (cmd != NULL) { 
							pid_t p = fork(); 
							if (p == 0) {
								if (execv(cmd, command_list[i]) < 0) {
									fprintf(stderr, "execv failed: %s\n", strerror(errno));
								} 
								//Code from Part 1 
								/*execv(command_list[i][j], command_list[i]); 
								//if (execv(command_list[i][j], command_list[i]) < 0) {
					       		//	fprintf(stderr, "execv failed: %s\n", strerror(errno));
								}*/
							}
							else if (p > 0) {
								wait(&p);
								free(command_list[i]); 
								//int status = 0;
								//waitpid(-1, &status, WNOHANG); 
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
						free_paths(shell_dir); //free linked list before exit
						free_tokens(command_chunks); //free tokens before exit
			    		exit(0);      
					} //end of if statement
					else if (strcasecmp(command_list[i][j], mode_word) == 0) { //mode
		   	    		j++;
						//print current mode if mode is not specified
			    		if (command_list[i][j] == NULL) { 
							printf("Current mode: %s\n", mode_arr[mode]);
			    		}
						//change mode to sequential 
			    		else if (strcasecmp(command_list[i][j], seq) == 0) { 
							change = 0;
			    		}
						//change mode to parallel
			    		else if (strcasecmp(command_list[i][j], par) == 0) { 
							change = 1;
			    		}
					} //end of else if
					else {
						pid_t pid = fork(); 
						int ran = 0;
						if (pid == 0) {
							//setpgid(0, 0);
							if (execv(command_list[i][j], command_list[i]) < 0) {
		           				fprintf(stderr, "execv failed: %s\n", strerror(errno));	
							}
							ran = 1;  
						}
						else if (pid > 0) {
							//int status = 0;
							//waitpid(-1, &status, WNOHANG); 
							if (ran == 1) {
								if (list == NULL)  {
									node *head = NULL; 
									head->p = pid;
									head->next = NULL; 
									list = head;	
								}
								else {
									node *tmp = NULL;
									tmp->p = pid;
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
