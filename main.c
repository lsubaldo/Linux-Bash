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
#include <signal.h>
#include <errno.h> 
#include <stdbool.h> 

//Authors: Cary Baun, Katie, Chungbin, and Leslie Subaldo
//Date: October 9, 2015 

typedef struct node {
	pid_t p;
	char command[32];
	int running; 
	struct node * next;
} node;

typedef struct directory {
	char dir[32];
	struct directory *next; 
} directory; 

/* main helper functions */

//counts number of entries in a linked list of nodes
int get_length (node *first) {
	int count = 0; 
	node *iter_node = first; 
	while (iter_node != NULL) {
		count++;
		iter_node = iter_node->next;
	}
	return count;
}

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

	//int size = sizeof(char*);
	char** tokens = malloc((count+1) * sizeof(char *));

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

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}

bool contains_pid(pid_t p, node *list) {
    node *iter_node = list; 
	bool rtv = false;    
	if (list == NULL) {
		printf("Job list empty.\n");
        return rtv;
    }

    while (iter_node != NULL) {
		if (iter_node->p == p) {
			return true;
		}
		iter_node = iter_node->next;
    }
	printf("Job list does not contain PID %d.\n", p);
	return rtv;
}

void pause_pid(pid_t p, node *list) {
	if (contains_pid(p, list)) {
	    node *iter_node = list; 
		while (iter_node->p != p) {
			iter_node = iter_node->next;
		}
		if (iter_node->p != 1) {
			iter_node->running = 1;
			kill(p, SIGSTOP);
		}
		else {
			printf("Process %d is already paused.", p);
		}
	}
}

void resume_pid(pid_t p, node *list) {
	if (contains_pid(p, list)) {
	    node *iter_node = list; 
		while (iter_node->p != p) {
			iter_node = iter_node->next;
		}
		if (iter_node->p != 0) {
			iter_node->running = 0;
			kill(p, SIGCONT);
		}
		else {
			printf("Process %d is already running.", p);
		}
	}
}

node *list_delete(pid_t p, node *list) {
    node *head = list; 
    if (head == NULL) {
        return list;
    }
    //if head is the node that is to be deleted
    else if (head->p == p) {
        list = list->next;
        free(head);
    }
    else { 
        while (head->next != NULL) {
              node *nxt = head->next;
              if (nxt->p == p) {
                  head->next = nxt->next; 
                  free(nxt);
                  break;
              }
              head = head->next;
        }
    }
    return list;
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
	char new[64];
	while (fgets(new, 64, fp) != NULL) {
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
         directory *path = paths; 
         paths = paths->next;
	 	free(path);
    }
}

//checks whether command is a valid file in a directory 
char *is_file(directory *dir_list, char *buf) {
	struct stat statresult; 
	while (dir_list != NULL) {
	//created temp string so that strcat would not mess with original path in linked list
		char *temp = malloc(sizeof(char) * 64); 
		strcpy(temp, dir_list->dir); 
		char *command = strcat(temp, buf);
		int rv = stat(command, &statresult);
		if (rv == 0) {
			return command;
		}
		dir_list = dir_list->next; 
		free(temp);
	} 
	printf("Error. Not a command.\n"); 
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

void remove_return(char* str) {
	int i = 0; 
	while (i < strlen(str)) {
		if (str[i] == '\r' || str[i] == '\n') {
			str[i] = '\0';
		}
	i++;
	}
}

void print_nodes(node *list) {
	char *status_arr[] = {"running", "paused"};

	if (get_length(list) == 0) {
		printf("No jobs running.\n");
	}
 	else {
		printf("Printing job list:\n");
		while (list != NULL) {
			printf("Process: %s, PID: %d, Status: %s\n", list->command, list->p, status_arr[list->running]);
			list = list->next;
		}
	}
}

void print_tokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

/* MAIN */ 
int main(int argc, char **argv) {

	directory *shell_dir = load_dir("shell-config"); 
	//list_print(shell_dir); 

	// mode settings: sequential = 0; parallel = 1; 
	int mode = 0;
	int change = 0;
	char *mode_arr[] = {"sequential", "parallel"};


	// Job list variables
	node *job_list = NULL; //FLAG LINKED LIST

	//Built-in function and mode variables 
	char *ex = "exit"; 
	char *mode_word = "mode";
	char *seq = "s";
	char *par = "p";
	char *jo = "jobs";
	char *pau = "pause";
	char *res = "resume";

    char buffer[1024];
	while (1) { // loop only exits through an exit() system call
		//command prompt loop

		if (mode == 0) { // Sequential Mode
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

			int i = 0;
	  	    while (command_list[i] != NULL) {
				if (command_list[i][0] == NULL) { // user enters only spaces
					printf("Not a valid command.\n");					
					i++;
				}	
				else {			
					int j = 0;
  					if (strcasecmp(command_list[i][j], ex) == 0) { //exit command called
			    		printf("Exit called. Goodbye.\n");
			    		fflush(stdout);
						free_paths(shell_dir);   //free linked list before exit
						free_tokens(command_chunks); //free tokens before exit
						int k = 0;
						while (k < chunk_count) {
							free_tokens(command_list[k]);
							k++;
						}
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
									fprintf(stderr, "Not a valid command. execv failed: %s\n", strerror(errno));
								} 
							}
							else if (p > 0) {
								wait(&p);
								free(cmd);  
							}
						}
					} //end of else
					i++;
				} //end of outer else 
			} //end of while loop
			free_tokens(command_chunks); //free tokens before asking for new commands
			int k = 0;
			while (k < chunk_count) {
				free_tokens(command_list[k]);
				k++;
			}
		} //end of sequential 

		else if (mode == 1) { //parallel mode*************************************

			char * prompt = "Prompt> ";
			printf("%s", prompt);
			fflush(stdout); //prints immediately 

		    struct pollfd pfd[1];
			pfd[0].fd = 0;
			pfd[0].events = POLLIN;
		    pfd[0].revents = 0;

		    int rv = poll(&pfd[0], 1, 10000);
			if (rv == 0) {
				printf("Timeout\n");
			}

			else if (rv > 0) { // must handle input
				//printf("You typed something on stdin\n");

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
				int i = 0;
				while (command_list[i] != NULL) { // iterate through line of commands
					if (command_list[i][0] == NULL) { // error case: user enters only spaces
						printf("Not a valid command.\n");
						i++;
					} // end of error case: only spaces
					else { // (non-error: user did not enter only spaces)
						int j = 0;

						/* BUILT-IN FUNCTIONS */

	  					if (strcasecmp(command_list[i][j], ex) == 0) { // exit command called
							if (get_length(job_list) > 0) { //job_list length > 0, can't exit
								printf("Jobs still running. Cannot exit.\n");
							}
							else {
								printf("Exit called. Goodbye.\n");
								fflush(stdout);
								free_paths(shell_dir); //free linked list before exit
								free_tokens(command_chunks); //free tokens before exit
								int i = 0;
								while (command_list[i] != NULL) {
									free_tokens(command_list[i]);
									i++;
								}
								exit(0);  
							}    
						} //end of exit case
					
						// mode command called
						else if (strcasecmp(command_list[i][j], mode_word) == 0) { 
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
						} //end of mode case

						//jobs command called
						else if (strcasecmp(command_list[i][j], jo) == 0) {
							print_nodes(job_list);
						}

						//pause command called
						else if (strcasecmp(command_list[i][j], pau) == 0) {
							j++;
							//pid_t tp = (int) command_list[i][j];
							pid_t tp = atoi(command_list[i][j]); 
							pause_pid(tp, job_list);
						}
						
						//resume command called
						else if (strcasecmp(command_list[i][j], res) == 0) {
							j++;
							//pid_t tp = (int) command_list[i][j];
							pid_t tp = atoi(command_list[i][j]); 
							resume_pid(tp, job_list);
						}

						/* END OF BUILT-IN FUNCTIONS */

						else { 
							char *cmd = NULL;
							bool has_slash = false;
							char *b = command_list[i][j];

							// Set proper path name
							for (int n=0 ; n<strlen(b); n++) { //test path name for slash
								if (b[n] == '/') {
									has_slash = true;
									break;
								}
							}
							if (has_slash) { //set cmd to b (runs as if it was in part 1)
								cmd = b; 
							}	
							else {  //check to make sure path is valid for part 2
								cmd = is_file(shell_dir, command_list[i][j]);
							}

							// Running commands
							if (cmd != NULL) { 
								pid_t pid = fork(); 
								if (pid == 0) { //child
									if (execv(cmd, command_list[i]) < 0) {
				       					fprintf(stderr, "Not a valid command. execv failed: %s\n", strerror(errno));	
									}
									free(cmd); 
								} // end of child
								else if (pid > 0) { // parent 
									printf("%d\n", pid);
									if (job_list == NULL) { //FLAG LINKED LIST
										node * head = malloc(sizeof(node));
										//node *head = NULL; 
										head->p = pid;
										remove_return(command_chunks[i]);
										strcpy(head->command, command_chunks[i]);
										head->running = 0;
										head->next = NULL; 
										job_list = head;	
									}
									else {
										node *tmp = malloc(sizeof(node));
										tmp->p = pid;
										remove_return(command_chunks[i]);
										strcpy(tmp->command, command_chunks[i]);
										tmp->running = 0;
										tmp->next = job_list;
										job_list = tmp;
									}
									free(cmd);  
								} //end of parent statement
							} //end of if cmd != NULL statement 
						} //end of non-built-in functions
						i++; //iterate through list of commands
					} //end of non-error outer else statement
				} //end of parallel while loop
			
				free_tokens(command_chunks); //free tokens before asking for new commands
				int k = 0;
				while (k < chunk_count) {
					free_tokens(command_list[k]);
					k++;
				}

			}//end of handle input
	
			else {
				printf("There was some kind of error: %s\n", strerror(errno)); 
			}


			// Modify job list if processes have exited FLAG LINKED LIST
			if (get_length(job_list) > 0) {
				int finished = waitpid(-1, NULL, WNOHANG);
				while (finished > 0) {	
					printf("Process %d done.\n", finished);
					job_list = list_delete(finished, job_list);
					finished = waitpid(-1, NULL, WNOHANG);
				}
			}
		} //end of parallel mode
		mode = change; // mode changes if 'change' has been modified
	} //end of big while loop
	return 0;
}		
