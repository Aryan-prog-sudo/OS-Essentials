#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h> //Includes pid_t (A type that contains PID)
#include <sys/wait.h> //Includes wait and waitpid function
#include <time.h> //Include features related to time

#define MAX_LINE_LENGTH 1024       
#define MAX_ARGS 100        
#define MAX_HISTORY 100  

struct HistoryEntry{
    char Command[MAX_LINE_LENGTH];
    pid_t PID;
    time_t StartTime;
    time_t EndTime;
};

struct HistoryEntry history[MAX_HISTORY];
int HistoryCount = 0;

char* UserInput();
char** parse_command(char *line);
int create_process_and_run(char *Input);
void execute_piped(char *line);
void print_history();
int launch(char *InputCommand);
void shell_loop();
void AddNullTermination(char *str);


void AddNullTermination(char *str) {
    //Remove the \n in the back of sommand with null termination
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1] = '\0';
}


void shell_loop(){
    int Status;
    do{
        printf("\nSimpleShell:$ ");
        
        char *Command = UserInput();
        if(Command==NULL||strcmp(Command, "")==0){
            continue;
        }
        if (strcmp(Command, "history") != 0 && strcmp(Command, "exit") != 0 && strcmp(Command, "Exit") != 0) {
            time(&history[HistoryCount].StartTime);
            strcpy(history[HistoryCount].Command, Command);
        }
        Status = launch(Command);
        if(Status==1 && strcmp(Command, "history") != 0 && strcmp(Command, "exit") != 0 && strcmp(Command, "Exit") != 0){
            time(&history[HistoryCount].EndTime);
            HistoryCount++;
        }
        free(Command);
    }while(Status);
}


char * UserInput(){
    char *InputCommand = malloc(MAX_LINE_LENGTH);
    if(InputCommand==NULL){
        printf("Memory allocation failed at User Input");
        exit(1);
    }
    fgets(InputCommand, MAX_LINE_LENGTH, stdin);
    int Length = strlen(InputCommand);
    AddNullTermination(InputCommand);
    return InputCommand;
}


int launch(char *InputCommand){
    if (strcmp(InputCommand, "history") == 0) {
        print_history();
        return 1;
    }
    if(strcmp(InputCommand, "exit")==0 ||strcmp(InputCommand, "Exit")==0){
        printf("Shell Terminated\n");
        print_history();
        return 0;
    }
    if (strchr(InputCommand, '|') != NULL) {
        execute_piped(InputCommand);
    } 
    else {
       pid_t ChildPID = create_process_and_run(InputCommand);
       if(ChildPID!=-1){
        history[HistoryCount].PID = ChildPID;
       }
    }
    return 1;
}


int create_process_and_run(char *Input){
    pid_t pid = fork(); //The fork returns 
    if(pid<0){
        printf("Something bad happened \n");
        exit(1);
    }
    else if(pid==0){
        char **Arguments = parse_command(Input);
        if(execvp(Arguments[0], Arguments)==-1){
            printf("exec failed");
            exit(1);
        }
    }
    else{
        int Status;
        waitpid(pid, &Status, 0);
        return pid;
    }
    return 0;
}


char **parse_command(char *line) {
    static char *args[MAX_ARGS+1]; 
    char *token;
    int i = 0;
    token = strtok(line, " ");
    while (token != NULL) {
        args[i] = token;
        i++;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // execvp needs NULL-terminated args
    return args;
}


void execute_piped(char *line) {
    char *commands[10];
    int NumberCommand = 0;
    
    char *line_copy = strdup(line); //Create a copy so as to not modify the orignial lina
    if (line_copy == NULL) {
        perror("strdup failed");
        return;
    }

    commands[NumberCommand++] = strtok(line_copy, "|");
    char *Token;
    while ((Token = strtok(NULL, "|")) != NULL) { //Break the pipe command into commands seperated by |
        commands[NumberCommand] = Token;
        NumberCommand++;
    }

    int pipefd[2]; //This array would go as input into the pipe()
    int InputFD = 0;
    pid_t last_pid = -1;
    //The 0 is the standard input value for file desciptor so the pipefd[0] is generally 0
    //The 1 is the standart output valur for file descripter so the pipefd[1] is generally 1
    for (int i = 0; i < NumberCommand; i++) { //Loop through all the commands and put them in a pipe
        if (i < NumberCommand - 1) {
            if (pipe(pipefd) == -1) {
                printf("pipe failed");
                return;
            }
        }
        pid_t pid = fork();
        if (pid < 0) { //Inside failed fork
            printf("fork failed");
            return;
        }

        if (pid == 0) { //Inside the child process
            if (InputFD != 0) {
                dup2(InputFD, 0);
                close(InputFD);
            }
            if (i < NumberCommand - 1) {
                dup2(pipefd[1], 1); //dup2 allocates new file desciptor
                close(pipefd[1]); //Close the output file descriptor
                close(pipefd[0]); //Close the input file descriptor
            }
            char *cmd = commands[i];
            while (*cmd == ' ') { //Skips white spaces in command as we add white spaces before and after | 
                cmd++;
            }
            char **args = parse_command(cmd); //Add NULL at the end for execvp
            if (execvp(args[0], args) == -1) { //execvp returns -1 value on failiure
                printf("execvp failed");
                exit(1);
            }
        }
        
        else { //Inside the parent class
            if (InputFD != 0) { //Close the input of the current pipe
                close(InputFD);
            }
            if (i < NumberCommand - 1) { //When more pipe seperated commands are remaining
                close(pipefd[1]);
                InputFD = pipefd[0];
            } 
            else { //If this was the last command
                close(pipefd[0]);
            }
            last_pid = pid; //Store the PID of the last child
        }
    }
    if (last_pid != -1) {
        int status;
        waitpid(last_pid, &status, 0); //Wait for the last process
        history[HistoryCount].PID = last_pid;
    }
    
    for (int i = 0; i < NumberCommand; i++) {
        // Wait for all child processes to prevent zombies
        wait(NULL);
    }

    free(line_copy);
}


void print_history() {
    printf("\nCommand History:-\n");
    for (int i = 0; i < HistoryCount; i++) {
        double TimeDifference = difftime(history[i].EndTime, history[i].StartTime);
        printf("(%d Command) PID=%d | CMD=\"%s\" | StartTime = %s| Endtime = %s| ExecutiionTime = %.2f\n" , i+1, history[i].PID, history[i].Command, ctime(&history[i].StartTime), ctime(&history[i].EndTime), TimeDifference);
    }
}


int main() {
    shell_loop();
    return 0;
}