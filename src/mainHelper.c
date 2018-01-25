#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include "sfish.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

pid_t currentPid;
int childStatus;

pid_t pipingPID;
int pipingChildStatus;

int numOfJobs = 1;
char* jobName;

pid_t runningPid;

void sigChildHandler(int signal);

typedef struct jobsToSuspend{
    int pid;
    int jobID;
    char jobName[500];
    struct jobsToSuspend* nextJob;
}node;

node* suspendedJobsHead = NULL;
node* suspendedJobsTail = NULL;

int findByPidAndJobNum(int pid, int jobNum){
    int exist = -1;
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        if(cursor -> pid == pid && cursor -> jobID == jobNum){
            return cursor -> pid;
        }
    }
    return exist;
}

void addSuspendedJob(int pid, char* jobName){
    node* jobToAdd = (node*)malloc(sizeof(node));
    jobToAdd -> pid = pid;
    jobToAdd ->jobID = numOfJobs;
    jobToAdd ->nextJob = NULL;
    memcpy(jobToAdd -> jobName, jobName, 500);
    if(suspendedJobsHead == NULL){
        suspendedJobsHead = jobToAdd;
        suspendedJobsTail = jobToAdd;
    }else{
        suspendedJobsTail -> nextJob = jobToAdd;
        suspendedJobsTail = suspendedJobsTail -> nextJob;
    }
    printf(JOBS_LIST_ITEM, numOfJobs,jobName);
    numOfJobs++;
}

void removeJobByPid(int pid){
    struct jobsToSuspend* cursor;
    cursor = suspendedJobsHead;
    if(cursor->pid == pid){
        if(cursor -> nextJob == NULL){
            suspendedJobsHead = NULL;
            suspendedJobsTail = NULL;
        }else{
            suspendedJobsHead = cursor -> nextJob;
        }
    }else{
        while(cursor -> nextJob != NULL){
            if(cursor -> nextJob -> pid == pid){
                if(cursor -> nextJob == suspendedJobsTail){
                    cursor -> nextJob = NULL;
                    suspendedJobsTail = cursor;
                    break;
                }else{
                    cursor -> nextJob = cursor -> nextJob -> nextJob;
                }
            }
            cursor = cursor -> nextJob;
        }
    }

    if(suspendedJobsHead == NULL){
        numOfJobs = 1;
    }
}

int getPidFromJID(int JID){
    int pid = -1;
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        if(cursor -> jobID == JID){
            return cursor -> pid;
        }
    }
    return pid;
}

char* getJobNameFromPID(int pid){
    char* rtrn = NULL;
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        if(cursor -> pid == pid){
            return cursor -> jobName;
        }
    }
    return rtrn;
}

int getJobIDFromPID(int pid){
    int jobID = -1;
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        if(cursor -> pid == pid){
            return cursor -> jobID;
        }
    }
    return jobID;
}

int ifPIDExists(int pid){
    int exist = -1;
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        if(cursor -> pid == pid){
            return cursor -> pid;
        }
    }
    return exist;
}

void printAllJobs(){
    struct jobsToSuspend* cursor;
    for(cursor = suspendedJobsHead; cursor != NULL; cursor = cursor -> nextJob){
        printf(JOBS_LIST_ITEM, cursor -> jobID,cursor -> jobName);
    }
}

int findOccurencesOfCharacter(char* string, char c){
    int count = 0;
    char *ptr = string;
    while(*ptr != '\0'){
        if(*ptr == c){
            count++;
        }
        ptr++;
    }
    return count;
}

void resumeProcess(int JID){
    int pid = getPidFromJID(JID);
    if(pid != -1){
        currentPid = pid;
        tcsetpgrp(0, getpgid(pid));
        kill(pid, SIGCONT);
        signal(SIGINT, sigChildHandler);
        waitpid(pid, &childStatus, WUNTRACED);
        if(WIFEXITED(childStatus) || WIFSIGNALED(childStatus)){
            sigChildHandler(SIGINT);
        }
        if(WIFSTOPPED(childStatus)){
            sigChildHandler(SIGTSTP);
        }
        tcsetpgrp(0, getpgid(getpid()));
    }
}

void killProcess(int JID){
    int pid = getPidFromJID(JID);
    if(pid != -1){
        kill(pid, SIGKILL);
        //////////////
        waitpid(pid, NULL, 0);
        ///////////////
        removeJobByPid(pid);
    }
}

void callExec(char *args[], char *currentDirectory){
    if(strcmp(args[0], "help") == 0){
        printf("%s\n", "Do you really need help? This is your own program so you should know what you are doing");
    }else if(strcmp(args[0], "pwd") == 0){
        printf("%s\n", currentDirectory);
    }else{
        if(execvp(args[0], args) == -1){
            printf(EXEC_NOT_FOUND, args[0]);
        }
    }
}

void sigChildHandler(int signal){
    if(signal == SIGINT){
        if(currentPid > 0){
            killProcess(getJobIDFromPID(currentPid));
            currentPid = -1;
        }
    }else if(signal == SIGTSTP){
        if(ifPIDExists(currentPid) == -1 && currentPid != -1){
            addSuspendedJob(currentPid, jobName);
            currentPid = -1;
        }else if(ifPIDExists(currentPid) != -1){
            printf("\n[%d] %s\n", getJobIDFromPID(currentPid), getJobNameFromPID(currentPid));
        }
    }
}

char* formatInput(char* input){
    char array[1000];
    char *ptr = input;
    char *arrayPtr = &array[0];

    while(*ptr != '\0'){
        if(*ptr == '<' || *ptr == '>' || *ptr == '|'){
            *arrayPtr = ' ';
            arrayPtr++;
            *arrayPtr = *ptr;
            arrayPtr++;
            *arrayPtr = ' ';
        }else{
            *arrayPtr = *ptr;
        }
        ptr++;
        arrayPtr++;
    }
    *arrayPtr = '\0';
    char* temp = array;
    return temp;
}

void executePrompt(char* input, char *currentDirectory){
    jobName = input;
    input = formatInput(input);

    char inputCopy[500];
    memcpy(inputCopy, input, sizeof(inputCopy));
    char *firstCommand;

    if(findOccurencesOfCharacter(input, '|') == 0){

        signal(SIGTSTP, sigChildHandler);
        signal(SIGINT, sigChildHandler);
        signal(SIGTTOU, SIG_IGN);

        if((currentPid = fork()) == 0){

            if(findOccurencesOfCharacter(inputCopy, '<') > 1 || findOccurencesOfCharacter(inputCopy, '>') > 1
                || strcmp(inputCopy,"<>") == 0 || strcmp(inputCopy, "><") == 0){
                fprintf(stderr,SYNTAX_ERROR, "Command not supported");
                exit(1);
            }

            char *args[50];
            int readingFileDescriptor;
            int writingFileDescriptor;
            int i = 0;
            firstCommand = strtok(inputCopy, " \t");

            setpgid(getpid(),getpid());
            tcsetpgrp(0, getpid());

            while(firstCommand != NULL){
                if(strcmp(firstCommand, "<") == 0){
                    firstCommand = strtok(NULL, " \t");
                    if((readingFileDescriptor = open(firstCommand, O_RDONLY, S_IRWXU)) < 0){
                        fprintf(stderr,EXEC_ERROR, "file not found");
                        exit(1);
                    }
                    dup2(readingFileDescriptor,0);
                    firstCommand = NULL;
                    close(readingFileDescriptor);

                }
                else if(strcmp(firstCommand, ">") == 0){
                    firstCommand = strtok(NULL, " \t");
                    writingFileDescriptor = creat(firstCommand, S_IRWXU);
                    dup2(writingFileDescriptor, 1);
                    firstCommand = NULL;
                    close(writingFileDescriptor);
                }
                args[i++] = firstCommand;
                firstCommand = strtok(NULL, " \t");
            }
            args[i] = NULL;

            callExec(args, currentDirectory);

            exit(EXIT_SUCCESS);

        }else{

            waitpid(currentPid, &childStatus, WUNTRACED | WCONTINUED);
            if(WIFEXITED(childStatus) || WIFSIGNALED(childStatus)){
                sigChildHandler(SIGINT);
            }
            if(WIFSTOPPED(childStatus)){
                sigChildHandler(SIGTSTP);
            }
            tcsetpgrp(0, getpgid(getpid()));
        }

    }else{

        int arrayLength = findOccurencesOfCharacter(input, '|') + 2;
        int numOfPipes = findOccurencesOfCharacter(input, '|') * 2;
        char* token = strtok(inputCopy,"|");
        char *temp[arrayLength];

        int i = 0;
        temp[i] = token;
        i++;
        while((token = strtok(NULL, "|")) != NULL){
            temp[i] = token + 1;
            i++;
        }
        temp[i] = NULL;

        int pipefd[numOfPipes];
        for(int i = 0; i < (numOfPipes/2); i++){
            pipe(pipefd + i*2);
        }

        int j = 0;
        int temp2Length;
        int pipeCounter = 0;
        signal(SIGCHLD, SIG_IGN);

        while(temp[j]!= NULL){
            temp2Length = 0;
            temp2Length = findOccurencesOfCharacter(temp[j], ' ') + 2;
            char *command1[temp2Length];
            int i = 0;
            token = strtok(temp[j], " \t");
            command1[i] = token;
            i++;

            while((token = strtok(NULL, " \t")) != NULL){
                command1[i] = token;
                i++;
            }
            command1[i] = NULL;

            if((pipingPID = fork()) == 0){

                if(temp[j + 1] != NULL){
                    dup2(pipefd[(pipeCounter*2) + 1], 1);
                }
                if(j != 0){
                    dup2(pipefd[(pipeCounter-1) * 2], 0);
                }
                for(int i = 0; i < numOfPipes; i++){
                    close(pipefd[i]);
                }
                callExec(command1, currentDirectory);
                exit(EXIT_SUCCESS);
            }
            j++;
            pipeCounter+=1;
        }
        for(int i = 0; i < numOfPipes; i++){
            close(pipefd[i]);
        }
        waitpid(pipingPID, &pipingChildStatus, WUNTRACED | WCONTINUED);
    }
}