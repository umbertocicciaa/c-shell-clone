#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

// sorteredOrder
char *builtInCommands[] = {"cd", "echo", "exit", "pwd", "type"};

char* filepathExecutable(char *command, char *path){
  if (path == NULL) return NULL;
  
  char *path_copy = strdup(path);
  char *dir_token = strtok(path_copy, ":");

  while (dir_token != NULL) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dir_token, command);
  
    if (access(filepath, X_OK) == 0) {
      char *result = strdup(filepath);
      free(path_copy);
      return result;
    }
    dir_token = strtok(NULL, ":");  
  }

  free(path_copy);
  return NULL;
}

int isBuiltIn(char *command){
  int left = 0, right = sizeof(builtInCommands) / sizeof(builtInCommands[0]) - 1;
  while (left <= right) {
    int mid = left + (right - left) / 2;
    int cmp = strcmp(command, builtInCommands[mid]);
    if (cmp == 0) {
      return 1;
    } else if (cmp < 0) {
      right = mid - 1;
    } else {
      left = mid + 1;
    }
  }
  return 0;
}

int main(int argc, char *argv[]){
  // Flush after every printf
  setbuf(stdout, NULL);

  char *path = getenv("PATH");

  // Read–Eval–Print Loop (REPL)
  while(1){
    printf("$ ");
    
    char command[1024];
    fgets(command, sizeof(command), stdin);
  
    command[strcspn(command, "\n")] = '\0';

    if (strcmp(command,"exit")==0){
      break;
    }

    else if (strncmp(command, "echo ", 5) == 0){
      printf("%s\n", command + 5);
    }

    else if (strncmp(command, "type ", 5) == 0){
      char *arg = command + 5;
    
      if (isBuiltIn(arg)==1) {
        printf("%s is a shell builtin\n", arg);
        continue;
      }
    
      if (path == NULL) {
        printf("%s: not found\n", arg);
        continue;
      }
      
      char* pathExecutable = filepathExecutable(arg, path);
      if (pathExecutable != NULL) {
        printf("%s is %s\n", arg, pathExecutable);
        free(pathExecutable);
      } else {
        printf("%s: not found\n", arg);
      }
      continue;
    }

    else if (strcmp(command,"pwd")==0 && getenv("PWD") != NULL){
      printf("%s\n", getenv("PWD"));
      continue;
    }

    else if (strncmp(command, "cd ", 3) == 0){
      char *arg = command + 3;
      if (strcmp(arg, "~") == 0) {
        char *home = getenv("HOME");
        if (home != NULL) arg = home;
      }
      struct stat sb;
      if (stat(arg, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        chdir(arg);
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
          setenv("PWD", cwd, 1);
        }
      } else {
        printf("cd: %s: No such file or directory\n", arg);
      }
    }

    else{
      char command_copy[1024];
      strncpy(command_copy, command, sizeof(command_copy) - 1);
      command_copy[sizeof(command_copy) - 1] = '\0';
      
      char *args[1024];
      int arg_count = 0;
      
      char *token = strtok(command_copy, " ");
      while (token != NULL && arg_count < 1023) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
      }
      args[arg_count] = NULL;
      
      if (arg_count > 0) {
        char* pathExecutable = filepathExecutable(args[0], path);
        
        if (pathExecutable != NULL) {
          pid_t pid = fork();
          if (pid == 0) {
            // Child process
            execv(pathExecutable, args);
            exit(1);
          } else if (pid > 0) {
            // Parent process
            wait(NULL);
          }
          free(pathExecutable);
        } else {
          printf("%s: command not found\n", args[0]);
        }
      }
    }
  }
  return 0;
}
