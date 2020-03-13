//copied from private directory
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<fcntl.h>
#define DELIMITERS " \t\r\n\a"

int numRedirect = 0;

//method declearations
int execute(char* args[], char* retArgs[]);

//path for binary files
typedef struct Node {
    
    char* data;
    struct Node *next;
    
} pathNode;

pathNode *head = NULL;

//insert link at the first location
void insertFirst(char* data) {
    
    //create a link
    pathNode *link = (pathNode*) malloc(sizeof(pathNode));
    link->data = data;
    
    //point it to old first node
    link->next = head;
    
    //point first to new first node
    head = link;
    
}

pathNode* deleteNode(char* data) {
    
    //start from the first link
    pathNode* current = head;
    pathNode* previous = NULL;
    
    //if list is empty
    if(head == NULL) {
        return NULL;
    }
    
    //navigate through list
    while(strcmp(current-> data, data) != 0) {
        
        //if it is last node
        if(current->next == NULL) {
            return NULL;
        } else {
            //store reference to current link
            previous = current;
            //move to next link
            current = current->next;
        }
    }
    
    //found a match, update the link
    if(current == head) {
        //change first to point to next link
        head = head->next;
    } else {
        //bypass the current link
        previous->next = current->next;
    }
    return current;
}

//clear list
void clearList(){
    
    pathNode* curr = head;
    pathNode* next;
    
    while(curr != NULL){
        next = curr -> next;
        free(curr);
        curr = next;
    }
    head = NULL;
    
}

//checks if path has / concatenated to it eg. /bin/ return 1 if true, otherwise return false
int checkBackslash(char *str){
    if(str[ strlen(str) - 1] == '/'){
        return 1;
    } else{
        return 0;
    }
}

void error_msg(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int command_cd(char* args[], int numArgs){
    if (numArgs != 2) {
        error_msg();
        return 1;
    }
    if (chdir(args[1]) == -1){
        error_msg();
        return 1;
    }
    return 0;
}


void printPath(){
    
    if(head == NULL){
        return;
    }
    
    pathNode* current = head;
    
    while(current != NULL){
        printf("%s\n", current -> data);
        current = current -> next;
    }
}

int command_path(char* args[], int numArgs){
    
    if(strcmp(args[1] , "add") == 0 ){
        //if array is full resize by twice the size
        if(checkBackslash(args[2]) == 1){
            
            args[2][strlen(args[2]) -1] = '\0';
            insertFirst(args[2]);
            
        } else{
            insertFirst(args[2]);
        }
    } else if(strcmp(args[1] , "remove") == 0 ){
      //  printf("args[2] : %s\n", args[2]); 
        deleteNode(args[2]);
        
    } else if(strcmp(args[1] , "clear") == 0){
        clearList();
    }

  //  printPath();
    
    return 0;
}

int lineSeperate(char* line, char* args[], char* delim) {
    char *save;
    int argsIndex = 0;
    
    if (!args){
        args = malloc(100*sizeof(char));
    }
    args[argsIndex] = strtok_r(line, delim, &save);
    argsIndex++;
    
    while(1){
        if (!(args + argsIndex)){
            char* temp = (char*)(args + argsIndex);
            temp  = malloc(100*sizeof(char));
            temp++;
        }
        args[argsIndex] = strtok_r(NULL, delim,&save);
        if (args[argsIndex] == NULL){
            break;
        }
        argsIndex++;
    }
    
    if (args[0] == NULL) return 0;
    
    return argsIndex;
}

int redirect(char* ret, char* line) {
    char* progArgs[100];
    char* retArgs[100];
    ret[0] = '\0';
    ret = ret + 1;
    int argsNum = lineSeperate(line, progArgs, DELIMITERS);
    if (argsNum == 0){
        error_msg();
        return 1;
    }
    int retArgc = lineSeperate(ret, retArgs, DELIMITERS);
    if (retArgc != 1){
        error_msg();
        return 1;
    }
    execute(progArgs, retArgs);
    return 0;
}

int parallel(char* ret, char* line){
    char** commands = malloc(100*sizeof(char*));
    int numCommands = lineSeperate(line, commands,"&");
    char** arguments = malloc(50*sizeof(char*));
    char* retRedir = malloc(100*sizeof(char));
    for (int i = 0; i < numCommands; i++) {
        if ((retRedir = strchr(commands[i], '>'))){
            redirect(retRedir, commands[i]);
            continue;
        }
        lineSeperate(commands[i], arguments, DELIMITERS);
        execute(arguments, NULL);
    }
    free(arguments);
    free(commands);
    return 0;
}


int multiple(char* ret, char* line){
    //store parsed commands
    char** arguments = malloc(50 * sizeof(char*));
    //stores commands parsed with ;
    char** cmds = malloc(100 * sizeof(char*));
    int numCommands = lineSeperate(line, cmds, ";");
    char* checkRedir = malloc(100* sizeof(char*));
    char* checkParal = malloc(100 * sizeof(char*));

    
    for(int i = 0; i < numCommands; i++){
        if ((checkParal = strchr(cmds[i], '&')) != NULL) {
            parallel(checkParal, cmds[i]);
            continue;
        }
        
        if((checkRedir = strchr(cmds[i], '>')) != NULL){
            redirect(checkRedir, cmds[i]);
            continue;
        }
        //store number of args after parsing by delimiter
        int numArgs = lineSeperate(cmds[i], arguments, DELIMITERS);
        //execute builtin command exit
        if(strcmp(arguments[0], "exit") == 0){
             //exit shell
            exit(0);
        } else if(strcmp(arguments[0], "cd") == 0){
            //execute cd
            command_cd(arguments, numArgs);
        } else if(strcmp(arguments[0], "path") == 0){
            //execute path
            command_path(arguments, numArgs);
        } else{
            execute(arguments, NULL);
        }
        
    }
    
    free(arguments);
    free(cmds);
    return 0;
}

int checkRedirect (char *line){
    for(int i = 0; i < strlen(line); ++i){
        if(line[i] == '>'){
            numRedirect++;
        }
    }
    return numRedirect;
}

int readCommand(char* args[],FILE * fp){
    
    //for user input
    char* checkRedir = NULL;
    char* checkParal = NULL;
    char* checkMul = NULL;
    char* line = malloc(100*sizeof(char));
    size_t len = 0;
    ssize_t nread;
    numRedirect = 0;
    
    
    fflush(stdin);
    if ((nread = getline(&line, &len, fp)) == -1) {
        //error_handler();
        return 1;
    }
    if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0)){
        return -1;
    }
    
    //omit the last \n of the string
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    //exit if EOF is read
    if (line[0] == EOF){
        return 1;
    }
    
    checkRedirect(line);
    
    if(numRedirect > 1){
     //  printf("%d\n", numRedirect);
        error_msg();
        return -1;
    }
    if((checkMul = strchr(line, ';')) != NULL){
        multiple(checkMul, line);
        return -1;
    }
    //for parallel commands
    if ((checkParal = strchr(line, '&')) != NULL){
        parallel(checkParal, line);
        return -1;
    }
    //for redirection,
    if ((checkRedir = strchr(line, '>')) != NULL){
        redirect(checkRedir, line);
        return -1;
    }
    //seperate the line
    int argsIndex = lineSeperate(line, args, DELIMITERS);
    if (argsIndex == 0) {
        return 0;
    }
    if (strcmp(args[0], "cd") == 0){
        command_cd(args, argsIndex);
        return -1;  //-1 for built-in command call flag
    }
    if (strcmp(args[0], "path") == 0){
        command_path(args, argsIndex);
        return -1;
    }
    //exit if requested
    if (strcmp(args[0], "exit") == 0){
        if (args[1]){
        error_msg();
        }
        exit(0);
    }
    return 0;
}

int execute(char* args[], char* retArgs[]){
    pid_t pid;
    char* commandPath = malloc(100*sizeof(char*));
    //test where is the expected executable file
    //index of PATH
    pathNode* currPath = head;
    
    int isNotFound = 1;
    if (head == NULL){
        error_msg();
        return 1;
    }
    if (args == NULL)
        return 1;
    if (args[0] == NULL)
        return 1;
    
    while(currPath != NULL) {
        if(currPath -> data == NULL)
            break;
        //copy the original string to a larger space
        char* tempStr = malloc(100*sizeof(char));
        if (!strcpy(tempStr, currPath -> data )){
            error_msg();
            return 1;
        }
        int strLen = strlen(tempStr);
        tempStr[strLen] = '/';
        tempStr[strLen + 1] = '\0';
        strcat(tempStr, args[0]);
        if (access(tempStr, X_OK) == 0){
            strcpy(commandPath, tempStr);
            isNotFound = 0;
            free(tempStr);
            break;
        }
        free(tempStr);
        currPath = currPath -> next;
    }
    if (isNotFound) {
        error_msg();
        return 1;
    }
    //fork and execute
    pid = fork();
  
    if (pid == -1){
            error_msg();
            return 1;
    } else if(pid == 0){
            if (retArgs){
                int fd_out = open(retArgs[0],O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                if (fd_out > 0){
                    //redirect STDOUT for this process
                    dup2(fd_out, STDOUT_FILENO);
                    dup2(fd_out, STDERR_FILENO);
                    
                    fflush(stdout);
                }
            }
            //the forked process will have a pid of 0 so it will execute  the file
            execv(commandPath, args);
    }
            
    else {
            //for father process it will goes to default and wait
            wait(NULL);
    }
    return 0;
}

int main(int argc, char* argv[]){
    
    int isBatchMode = 0;
    char* initialPath = "/bin";
    
    pathNode *path = (pathNode *)malloc(sizeof(pathNode));
    path -> data = strdup(initialPath);
    path -> next = NULL;
    head = path;
    
    FILE *fp;
    char** args;
    //char* userArgs[100] = &argv[1];
    if (argc == 2) {
        if (!(fp = fopen(argv[1], "r")) ){
            error_msg();
            exit(1);
        }
        isBatchMode = 1;
    } else if (argc < 1 || argc > 2) {
        error_msg();
        exit(1);
    }
    int isFinish = 0;
    while(1) {
        
        if (isBatchMode == 1){
            while(1){
                args = malloc(100*sizeof(char));
                int readStatus = readCommand(args, fp);
                fflush(fp);
                if (readStatus == -1)
                    continue;
                if (readStatus == 1) {
                    isFinish = 1;
                    break;
                }
                int errNum = execute(args, NULL);
                free(args);
                if (errNum == 1)
                    continue;
            }
            break;
        } else {
            fprintf(stdout, "smash> ");
            fflush(stdout);
            args = malloc(100*sizeof(char));
            int readStatus = readCommand(args, stdin);
            fflush(stdin);
            if (readStatus == -1)
                continue;   //if a built-in command is called, continue;
            if (readStatus == 1)
                break;
            if (execute(args, NULL) == 1)
                continue;   //if there is an error, continue
            free(args);
        }
        if (isFinish)
            break;
    }
    return 0;
}

