#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <sys/wait.h>

#include "sfish.h"
#define COLOR "debug.h"
#include "debug.h"

void executePrompt(char* input, char* currentDirectory);
void sigMainHandler(int signal){
    // printf("%s\n", "going to main handler");
};
void printAllJobs();
int findOccurencesOfCharacter(char* string, char c);
void resumeProcess(int JID);
void killProcess(int JID);
int ifPIDExists(int pid);
int getJobIDFromPID(int pid);
void redirection(char* input, char *currentDirectory);
int getPidFromJID(int JID);

int main(int argc, char *argv[], char* envp[]) {
    char* input;
    bool exited = false;

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }

    char currentDirectory[500];
    char inputCopy[500];
    char lastDirectory[500];
    int removeLength;
    int currentDirectoryLength ;

    signal(SIGINT, sigMainHandler);
    signal(SIGTSTP, sigMainHandler);

    char color[5];

    do {
        char prompt[1000];
        getcwd(currentDirectory, 500);

        if(strstr(currentDirectory, getenv("HOME")) != NULL){
            memcpy(inputCopy, currentDirectory, sizeof(currentDirectory));
            memset(inputCopy, '\0', strlen(getenv("HOME")));

            if(strcmp(color, "KNRM") == 0){
                memcpy(prompt, "\033[0m\0", strlen("\033[0m") + 1);
            }else if(strcmp(color, "KRED") == 0){
                memcpy(prompt, "\033[1;31m\0", strlen("\033[1;31m") + 1);
            }else if(strcmp(color, "KGRN") == 0){
                memcpy(prompt, "\033[1;32m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KYEL") == 0){
                memcpy(prompt, "\033[1;33m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KBLU") == 0){
                memcpy(prompt, "\033[1;34m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KMAG") == 0){
                memcpy(prompt, "\033[1;35m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KCYN") == 0){
                memcpy(prompt, "\033[1;36m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KWHT") == 0){
                memcpy(prompt, "\033[1;37m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KBWN") == 0){
                memcpy(prompt, "\033[1;38m\0", strlen("\033[1;31m") + 1);
            }else{
                memcpy(prompt, "\033[0m\0", strlen("\033[0m\0") + 1);
            }
            strcat(prompt, "~");
            strcat(prompt, inputCopy + strlen(getenv("HOME")));
            strcat(prompt, " :: mnisan>> ");
            strcat(prompt, KNRM);
        }else{
            // memcpy(prompt, KRED, strlen(KRED));
            // strcat(prompt, currentDirectory);
            if(strcmp(color, "KNRM") == 0){
                memcpy(prompt, "\033[0m\0", strlen("\033[0m") + 1);
            }else if(strcmp(color, "KRED") == 0){
                memcpy(prompt, "\033[1;31m\0", strlen("\033[1;31m") + 1);
            }else if(strcmp(color, "KGRN") == 0){
                memcpy(prompt, "\033[1;32m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KYEL") == 0){
                memcpy(prompt, "\033[1;33m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KBLU") == 0){
                memcpy(prompt, "\033[1;34m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KMAG") == 0){
                memcpy(prompt, "\033[1;35m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KCYN") == 0){
                memcpy(prompt, "\033[1;36m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KWHT") == 0){
                memcpy(prompt, "\033[1;37m\0", strlen("\033[1;31m") + 1);

            }else if(strcmp(color, "KBWN") == 0){
                memcpy(prompt, "\033[1;38m\0", strlen("\033[1;31m") + 1);
            }else{
                memcpy(prompt, "\033[0m\0", strlen("\033[0m\0") + 1);
            }
            strcat(prompt, currentDirectory);
            strcat(prompt, " :: mnisan>> ");
            strcat(prompt, KNRM);
        }

        input = readline(prompt);

        if(input == NULL){
            printf("\n");
            continue;
        }

        // write(1, "\e[s", strlen("\e[s"));
        // write(1, "\e[20;10H", strlen("\e[20;10H"));
        // write(1, "SomeText", strlen("SomeText"));
        // write(1, "\e[u", strlen("\e[u"));

        memcpy(inputCopy, input, sizeof(inputCopy));//changes if inputCopy size changes

        char *firstCommand = strtok(inputCopy, " \t");

        if(strlen(input) == 0 || firstCommand == NULL){
            continue;
        }
        if(strcmp(firstCommand, "exit") == 0){
            exit(0);
        }else if(strcmp(firstCommand, "cd") == 0){
            char *secondCommand = strtok(NULL, " \t");
            if(secondCommand == NULL){
                memcpy(lastDirectory,currentDirectory, 500);
                chdir(getenv("HOME"));
            }else if(strcmp(secondCommand, "-") == 0){
                chdir(lastDirectory);
                memcpy(lastDirectory,currentDirectory, 500);
            }else if(strcmp(secondCommand, ".") == 0){
                memcpy(lastDirectory,currentDirectory, 500);
            }else if(strcmp(secondCommand, "..") == 0){
                memcpy(lastDirectory,currentDirectory, 500);
                removeLength = strlen(strrchr(currentDirectory, '/'));
                currentDirectoryLength = strlen(currentDirectory);
                if(strcmp(strchr(currentDirectory, '/'), strrchr(currentDirectory, '/')) == 0){
                    memset(currentDirectory + 1, '\0', 1);
                }else{
                    memset((currentDirectory + (currentDirectoryLength - removeLength)), '\0', 1);
                }
                chdir(currentDirectory);
            }else{
                if(chdir(secondCommand) != -1){
                    memcpy(lastDirectory,currentDirectory, 500);
                }else {
                    printf(BUILTIN_ERROR, "Not a valid directory");
                }
            }
        }else if(strcmp(firstCommand, "jobs") == 0){
            printAllJobs();
        }else if(strcmp(firstCommand, "fg") == 0){
            char *secondCommand = strtok(NULL, " \t");
            if(findOccurencesOfCharacter(secondCommand, '%') == 1){
                int JID = atoi(secondCommand+1);
                if(ifPIDExists(getPidFromJID(JID)) != -1){
                    resumeProcess(JID);
                }else{
                    printf(BUILTIN_ERROR, "no such job");
                }
            }else{
                printf(BUILTIN_ERROR, "improper use of fg");
            }
        }else if(strcmp(firstCommand, "kill") == 0){
            char *secondCommand = strtok(NULL, " \t");
            if(findOccurencesOfCharacter(secondCommand, '%') == 1){
                int JID = atoi(secondCommand+1);
                if(ifPIDExists(getPidFromJID(JID)) != -1){
                    killProcess(JID);
                }else{
                    printf(BUILTIN_ERROR, "no such job");
                }
            }else if(findOccurencesOfCharacter(secondCommand, '%') == 0){
                int PID = atoi(secondCommand);
                if(ifPIDExists(PID) != -1){
                    killProcess(getJobIDFromPID(PID));
                }else{
                    printf(BUILTIN_ERROR, "no such job");
                }
            }else{
                printf(BUILTIN_ERROR, "improper use of kill");
            }
        }else if(strcmp(firstCommand, "color") == 0){
            char *secondCommand = strtok(NULL, " \t");
            if(strcmp(secondCommand, "RED") == 0){
                memcpy(color, "KRED", 5);
            }else if(strcmp(secondCommand, "GRN") == 0){
                memcpy(color, "KGRN", 5);
            }else if(strcmp(secondCommand, "YEL") == 0){
                memcpy(color, "KYEL", 5);
            }else if(strcmp(secondCommand, "BLU") == 0){
                memcpy(color, "KBLU", 5);
            }else if(strcmp(secondCommand, "MAG") == 0){
                memcpy(color, "KMAG", 5);
            }else if(strcmp(secondCommand, "CYN") == 0){
                memcpy(color, "KCYN", 5);
            }else if(strcmp(secondCommand, "WHT") == 0){
                memcpy(color, "KWHT", 5);
            }else if(strcmp(secondCommand, "BWN") == 0){
                memcpy(color, "KBWN", 5);
            }else{
                printf(BUILTIN_ERROR, "invalid color");
            }

        }else{
            executePrompt(input, currentDirectory);
        }

        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;
}

