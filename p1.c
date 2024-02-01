#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// p1.c
// Joe McDonald
// CSC360 - p1
// Oct 2, 2023

struct BackgroundProcess { //struct for background processes info
    pid_t PID;
    char **argument;
    int argument_size;
};

typedef struct BackgroundNode { //use linked list so you can have unlimited processes
    struct BackgroundProcess process;
    struct BackgroundNode* next;
} BackgroundNode;

int process_index = 1; 

void cd_helper(char** user_input_array) {
    if (user_input_array[1] == NULL || (strcmp(user_input_array[1], "~") == 0)) { //if they type "cd" or "cd ~" go to home directory
        const char *user = getenv("USER");
        if (user != NULL) {
            char dir[256];
            snprintf(dir, sizeof(dir), "/home/%s", user);
            if (chdir(dir) != 0) {
                perror("chdir");
                printf("Error");
            }
        }
    } else if (strcmp(user_input_array[1], "..") == 0) { //if they type "cd .." go back a directory
        chdir("..");
    } else if (chdir(user_input_array[1]) == -1) {
        printf("cd: %s: no such file or directory\n", user_input_array[1]);
    } else { //go to the specified directory TODO: add case for unknown directory
        chdir(user_input_array[1]);
    }
}

int main() {
    const char *username = getenv("USER"); //get hostname
    char hostname[250];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Unable to retrieve the hostname");
        return 1;
    }
    char prompt[700];
    BackgroundNode* head = NULL; //initialize to null
    int bailout = 0;
    while (!bailout) {
        BackgroundNode* previous = NULL; //initializing
        BackgroundNode* current = head; //initializing
        int terminated_index = 1;
        while (current) {
            // temporarily removed
        }
        char directory[400]; //create a string for directory
        getcwd(directory, sizeof(directory));
        snprintf(prompt, sizeof(prompt), "%s@%s:%s>", username, hostname, directory);
        printf("%s", prompt);
        fflush(stdout); //flush the output whenever enter is hit

        char *user_input = NULL; //initialize user input string
        size_t user_input_size = 0;
        ssize_t temp = getline(&user_input, &user_input_size, stdin); //get the user input
        if (temp == -1) {
            perror("getline");
            exit(EXIT_FAILURE);
        }
        user_input_size = strlen(user_input);
        if (user_input_size > 1) { //empty user inputs dont enter this statement
            if (user_input[temp - 1] == '\n') { //remove new line from user input
                user_input[temp - 1] = '\0';
                user_input_size--;
            }
            char **user_input_array = NULL;
            int user_input_array_size = 0;
            char *token = strtok(user_input, " "); //seperate user input by spaces
            while (token != NULL) {
                user_input_array = realloc(user_input_array, (user_input_array_size + 1) * sizeof(char *)); //assign memory for user input
                if (user_input_array == NULL) {
                    perror("Cannot allocate memory");
                    exit(EXIT_FAILURE);
                }
                user_input_array[user_input_array_size] = strdup(token);
                if (user_input_array[user_input_array_size] == NULL) {
                    perror("Cannot allocate memory");
                    exit(EXIT_FAILURE);
                }
                user_input_array_size++;
                token = strtok(NULL, " ");
            }
            user_input_array = realloc(user_input_array, (user_input_array_size + 1) * sizeof(char *)); //save user input
            if (user_input_array == NULL) {
                perror("Memory Allocation Failure");
                exit(EXIT_FAILURE);
            }
            user_input_array[user_input_array_size] = NULL; //null terminator
            if (user_input_array != NULL) {
                if (strcmp(user_input_array[0], "cd") == 0) {
                    cd_helper(user_input_array); //helper function
                } else if (strcmp(user_input_array[0], "bg") == 0) {
                    if (user_input_array[1] != NULL) { //if there is input after bg
                        pid_t childprocess = fork();
                        if (childprocess == -1) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }
                        if (childprocess == 0) {
                            int garbage_bin = open("/dev/null", O_WRONLY); //open this for writing
                            dup2(garbage_bin, STDOUT_FILENO); //put the output of child process here so it is not outputed
                            close(garbage_bin); //close file to prevent issues
                            char *fixed_argument[110];
                            int fixed_count = 0;
                            for (int i = 1; user_input_array[i] != NULL; i++) {
                                fixed_argument[fixed_count] = user_input_array[i];
                                fixed_count++;
                            }
                            fixed_argument[fixed_count] = NULL;
                            if (execvp(user_input_array[1], fixed_argument) == -1) {
                                perror("execvp");
                                exit(EXIT_FAILURE);
                            }
                        } else { //add a new node to BackgroundNode
                            BackgroundNode* new = (BackgroundNode*) malloc(sizeof(BackgroundNode)); //allocate memory for this new node
                            new->process.argument = malloc((user_input_array_size + 2) * sizeof(char *)); //allocate enough space for the arguments (because of it's unknown length)
                            new->process.PID = childprocess; //childprocess is just the PID
                            new->process.argument_size = user_input_array_size; //should be same
                            memset(new->process.argument, 0, (user_input_array_size + 2) * sizeof(char *)); //fill the memory for new with 0 to prevent issues
                            for (int i = 1; i < user_input_array_size; i++) { //use i=1 to prevent "bg" from being added
                                new->process.argument[i-1] = strdup(user_input_array[i]); //copy the user input array over to the node
                                if(new->process.argument[i-1] == NULL) { //if for some reason something doesnt work
                                    perror("Memory Allocation Failed");
                                    exit(EXIT_FAILURE);
                                }
                            }
                            new->process.argument[user_input_array_size-1] = NULL; //null terminator
                            new->next = head; //have next equal null
                            head = new;
                            process_index++;
                        }
                    } else { //if the user enters just "bg"
                        printf("You need to add arguments after bg\n");
                    }
                } else if (strcmp(user_input_array[0], "bglist") == 0) {
                    printf("Background processes: \n");
                    current = head;
                    int i = 1;
                    while (current) { //go through all the background processes
                        printf("%d: %s ", current->process.PID, directory);
                        char **temp_argument = current->process.argument;
                        while (*temp_argument) { //print all the arguments
                            printf("%s ", *temp_argument);
                            temp_argument++;
                        }
                        printf("%d\n", i++); //print the number in the list and increment the index variable
                        current = current->next; //increment to next node
                    }
                    printf("Total background jobs: %d\n", i-1); //i-1 is the number of the last background job
                } else if (strcmp(user_input_array[0], "bye") == 0) {
                    bailout = 1; //leave program
                } else { //normal commands eg. ls, ip -address etc.
                    pid_t childprocess = fork(); //create process
                    if (childprocess == -1) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                    if (childprocess == 0) {
                        if (execvp(user_input_array[0], user_input_array) == -1) { //execute the process and enter statement if it fails
                            perror("execvp"); //error handling if command execution fails
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        int status;
                        waitpid(childprocess, &status, 0); //no waiting
                        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) { //if exit status is not normal, capture error message
                            fprintf(stderr, "Failure with code: %d\n", WEXITSTATUS(status)); //print error message
                        }
                    }
                }
            }
            int i = 0;
            while (i < user_input_array_size) { //free the user_input_array memory
                free(user_input_array[i]);
                i++;
            }
            free(user_input_array);
        }
        free(user_input);
    }
    while (head) { //free memory allocated for all the nodes
        BackgroundNode* temp = head;
        head = head->next;
        for (int j = 0; j < temp->process.argument_size; j++) { //free the whole process node
            free(temp->process.argument[j]);
        }
        free(temp->process.argument); //free argument itself
        free(temp); //free the node itseld
    }
    printf("Bye Bye %s\n", username);
    return 0;
}