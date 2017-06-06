#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

char* args[256];                      //user input
int argsSize = 256;

char* history[10] = {NULL};           //history
int hisNum = 0;

int isPipe = false;                   //Used to check for pipes

void typePrompt(){
  char hostname[1024];
  char cwd[1024];
  gethostname(hostname, sizeof(hostname));
  printf("%s@%s:~%s$ ", getenv("LOGNAME"), hostname, getcwd(cwd,sizeof(cwd)));
}

void update_history(char* line){
  if (hisNum < 10){
    history[hisNum] = line;
  }
  else{
    int i;
    for (i = 0; i < 9; i++){
      history[i] = history[i + 1];
    }
    history[9] = line;
  }
}

void readCommand(){                   //Parses user_input into args[]
  char line[1024];
  int numTokens = 1;

  memset(line, '\0', sizeof(line));   //memset(ptr, value, size);
  fgets(line, sizeof(line), stdin);   //fgets(ptr, size, stream);

  if (strchr(line, '|') != NULL){     //Pipe check
    isPipe = true;
  }

  update_history(strdup(line));       //history
  hisNum++;

  args[0] = strtok(line, " \n\t");    //strtok(str, delim);
  while((args[numTokens] = strtok(NULL, " \n\t")) != NULL){
    numTokens++;
  }
}

int findNumCommands(){                    //Assumes numCommands = numPipes + 1;
  int i;
  int numCommands = 0;

  for (i = 0; i < argsSize; i++){
    if (args[i] != NULL){
      if (strcmp(args[i], "|") == 0){
        numCommands++;
      }
    }
    else{
      break;
    }
  }
  numCommands++;
  return numCommands;
}

void execPipeCommand(){

  int pipeO[2]; //odd
  int pipeE[2]; //even

  int numCommands = findNumCommands();
  char* commands[256];                                  //Commands stores each pipe instruction
  int start, i, j;
  start = 0;

  for(i = 0; i < numCommands; i++){                     //For loop used to iterate through each args

    for(j = 0; j < argsSize; j++){                      //For loop used to parse args into commands[]
      if((args[start] == NULL) || (strcmp(args[start], "|") == 0)){
        start++;
        commands[j] = NULL;
        break;
      }
      else{
        commands[j] = args[start];
        start++;
      }
    }

    if(i%2 == 0){
      pipe(pipeE);
    }
    else{
      pipe(pipeO);
    }

    pid_t pid;
    pid = fork();

    if (pid < 0){
      printf("Fork Failed\n");
      return;
    }

    else if (pid == 0){                                 //CHILD PROCESS

      if (i == 0){                                      //First command
        dup2(pipeE[1], 1);
        close(pipeE[0]);
      }

      else if (i == numCommands - 1){                   //Last command
        if(numCommands%2 == 0){
          dup2(pipeE[0], 0);
          close(pipeE[1]);
        }
        else{
          dup2(pipeO[0], 0);
          close(pipeO[1]);
        }
      }

      else{                                             //Middle command
        if (i%2 == 0){
            dup2(pipeE[1], 1);
            dup2(pipeO[0], 0);
            close(pipeE[0]);
            close(pipeO[1]);
        }
        else{
            dup2(pipeE[0],0);
            dup2(pipeO[1],1);
            close(pipeE[1]);
            close(pipeO[0]);
        }
      }

      if(execvp(commands[0], commands) == -1){
        printf("Command not Found\n");
        exit(0);
      }
    }

    else{
      if(i == 0){
        close(pipeE[1]);                              //First Parent
      }

      else if(i == numCommands -1){                   //Last Parent
        if(numCommands%2 == 0){
          close(pipeE[0]);
        }
        else{
          close(pipeO[0]);
        }
      }

      else{                                           //Middle Parent
        if(i%2 == 0){
          close(pipeO[0]);
          close(pipeE[1]);
        }
        else{
          close(pipeE[0]);
          close(pipeO[1]);
        }
      }
    }
  }

  //Closes all pipes
  close(pipeE[0]);
  close(pipeE[1]);
  close(pipeO[0]);
  close(pipeO[1]);

  for (i = 0; i < numCommands; i++){
    wait(NULL);
  }
}

void execCommand(){

  //Pipes
  if (isPipe == true){
    execPipeCommand();
    isPipe = false;
  }

  //EXIT
  else if(strcmp(args[0], "exit") == 0){
    exit(0);
  }

  //PWD
  else if(strcmp(args[0], "pwd") == 0){
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
      printf("%s\n", cwd);
    }
  }

  //CD
  else if(strcmp(args[0], "cd") == 0){
    if (args[1] == NULL){
      chdir(getenv("HOME"));
    }
    else{
      if(chdir(args[1]) == -1){
        printf("No such file or directory\n");
      }
    }
  }

  //History
  else if(strcmp(args[0], "history") == 0){
    int i;
    for (i = 0; i < 10; i++){
      if(history[i] != NULL){
        printf("  %d  %s", i+1, history[i]);
      }
    }
  }

  //External Program
  else{
    pid_t pid;

    pid = fork();

    if (pid < 0){
      fprintf(stderr, "Fork Failed\n");
    }

    else if (pid == 0){   //child
      if (execvp(args[0], args) == -1){
        printf("Command not found\n");
        exit(0);
      }
    }
    else{
      wait(NULL);
    }
  }
}

int main(){
  while(true){
    typePrompt();
    readCommand();
    if( args[0] != NULL){
      execCommand();
    }
  }
}
