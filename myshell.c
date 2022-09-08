#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h> 
#include <errno.h>




struct cmd_args {
    char *val;
    struct cmd_args *next;
};

struct cmd { 
    char *cmd_name;
    char *redirect;
    struct cmd_args *cmd_args;
    struct cmd *next;
    char redirect_err;
};

void foo(long int a){
    return;
}

void myPrint(char *msg);
void myError();
struct cmd_args *cmdinit(char *val, struct cmd_args *next);
struct cmd *getCmd(char *inp);
void show_cmds(struct cmd *a);
void free_cmd(struct cmd *a);
char **arglistarr(struct cmd *a, int* count);
void toolongcmd(char *msg, FILE *fd);
void show_args(struct cmd_args *a);
void batchPrint(char *msg);

int main(int argc, char *argv[]) 
{
    char cmd_buff[514] = {0};
    char *pinput;
    char **newargv = NULL;

    FILE *fd = fopen(argv[1], "r");

    while (1) {
        if(argc == 1){
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 514, stdin);
        } else {
            pinput = fgets(cmd_buff, 514, fd);
        }

        if(cmd_buff[512] != '\n' && cmd_buff[512] != 0){
            toolongcmd(cmd_buff, fd);
            memset(cmd_buff, 0, 514);
            continue;
        } 

        if(argc != 1){
            batchPrint(cmd_buff);
        }

        if (!pinput) {
            exit(0);
        }

        struct cmd *a = getCmd(cmd_buff);
        struct cmd *curr = a;

        show_cmds(a);

        while(curr != NULL && curr->cmd_name != NULL){
            if(!strcmp(curr->cmd_name, "exit"))
                if(curr->cmd_args != NULL || curr->redirect != NULL || curr->redirect_err != 0)
                    myError();
                else
                    exit(0);
            else if(!strcmp(curr->cmd_name, "pwd")){
                if(curr->cmd_args != NULL || curr->redirect != NULL || curr->redirect_err != 0)
                    myError();
                else{
                    char wdbuff[PATH_MAX] = {0};
                    getcwd(wdbuff, sizeof(wdbuff));
                    myPrint(wdbuff);
                    myPrint("\n");
                }
            }  
            else if(!strcmp(curr->cmd_name, "cd")){
                int out = 0;
                if(curr->redirect != NULL || curr->redirect_err != 0){
                    myError();
                } else if(curr->cmd_args == NULL){
                    out = chdir(getenv("HOME"));
                } else if(curr->cmd_args->next != NULL){
                    myError();
                } else {
                    out = chdir(curr->cmd_args->val);
                    if(out){
                        myError();
                    }
                }  
            } else if(!isalnum(curr->cmd_name[0])){
                curr = curr->next;
                continue;
            } else {
                int newargc = 0;
                newargv = arglistarr(curr, &newargc);
                int x = 0, status = 0;
                if((x = fork()) == 0){
                    if(curr->redirect != NULL){
                        int fd = open(curr->redirect, O_CREAT | O_RDWR | O_TRUNC);
                        dup2(fd, STDOUT_FILENO);
                    }
                    execvp(newargv[0], newargv);
                    myError();
                    exit(0);
                } else {
                    while((wait(&status) > 0));
                    // free(newargv);
                }
            }
            curr = curr->next;
        }
        //free_cmd(a);
        memset(cmd_buff, 0, 514);
    }
    return 0;
}

void batchPrint(char *msg)
{
    int print = 0;
    int i = 0;
    while(msg[i] != 0){
        if((msg[i] >= 33) && (msg[i] <= 126))
            print += 1;
        i++;
    }
    if(print)
        write(STDOUT_FILENO, msg, strlen(msg));   
}

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void myError()
{
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

struct cmd_args *cmdinit(char *val, struct cmd_args *next)
{
    struct cmd_args *a = (struct cmd_args *)malloc(sizeof(struct cmd_args));
    a->val = val;
    a->next = next;
    return a;
}

void show_cmds(struct cmd *a)
{
    while(a != NULL){
        fprintf(stderr, "%s\n", a->cmd_name);
        struct cmd_args *tmp = a->cmd_args;
        while(tmp != NULL){
            fprintf(stderr, "%s\n", tmp->val);
            tmp = tmp->next;
        }
        fprintf(stderr, "%d %s\n", a->redirect_err, a->redirect);
        a = a->next;
        printf("next \n");
    }
}

void show_args(struct cmd_args *a)
{
    struct cmd_args *tmp = a;
    while(tmp != NULL){
        fprintf(stderr, "%s\n", tmp->val);
        tmp = tmp->next;
    }
}

void free_cmd(struct cmd *a)
{
    while(a != NULL){
        struct cmd_args *tmp = a->cmd_args;
        while(tmp != NULL){
            struct cmd_args *tmp2 = tmp->next;
            free(tmp->val);
            free(tmp);
            tmp = tmp2;
        }
        free(a->cmd_name);
        struct cmd *curr = a;
        a = a->next;
        free(curr);
    }
}

struct cmd *getCmd(char *inp)
{
    struct cmd *a = (struct cmd *)malloc(sizeof(struct cmd));
    struct cmd *out = a;
    a->cmd_args = NULL;
    a->redirect = NULL;
    a->redirect_err = 0;
    
    char buff[514] = {0};
    int i = 0;

    while(inp[i] != '\n'){
        while((inp[i] == ' ' || inp[i] == '\t' ) && inp[i] != ';' && inp[i] != '>')
            i++;

        
        if(inp[i] == ';'){
            i++;
            continue;
        }

        if(inp[i] == '>'){
            i++;
            a->redirect_err = 1;
            goto redirect;
        }

        int j = 0;

        while(inp[i] != ' ' && inp[i] != '\t' && inp[i] != 0 && inp[i] != '\n' && inp[i] != ';' && inp[i] != '>')
            buff[j++] = inp[i++];

        
        a->cmd_name = strdup(buff);

        if(inp[i] == '>'){
            i++;
            goto redirect;
        }

        struct cmd_args *b = cmdinit(NULL, NULL);
        struct cmd_args *curr = b;
        memset(buff, 0, 514);

        while(inp[i] != '\n' && inp[i] != ';' && inp[i] != '>'){
            struct cmd_args *tmp = curr;
            curr->next = cmdinit(NULL, NULL);
            curr = curr->next;

            j = 0;

            while((inp[i] == ' ' || inp[i] == '\t') && inp[i] != '>')
                i++;


            while(inp[i] != ' ' && inp[i] != '\t' && inp[i] != 0 && inp[i] != '\n' && inp[i] != ';' && inp[i] != '>'){
                buff[j++] = inp[i++];
            }

            if(strcmp(buff, "")){
                curr->val = strdup(buff);
            } else {
                tmp->next = NULL;
                free(curr);
            }
            memset(buff, 0, 514);
        }

        curr = b->next;
        free(b);
        a->cmd_args = curr;

        if(inp[i] == '>'){
            i++;
            goto redirect;
        }
            
        if(inp[i] == ';'){
            i++;
            a->next = (struct cmd *)malloc(sizeof(struct cmd));
            a = a->next;
            a->cmd_name = NULL;
            a->cmd_args = NULL;
            a->redirect = NULL;
            a->next = NULL;
            a->redirect_err = 0;
        }

        memset(buff, 0, 514);
        continue;

        redirect:
            memset(buff, 0, 514);
            while(inp[i] == ' ' || inp[i] == '\t')
                i++;

            if(inp[i] == '>')
                a->redirect_err = 1;

            j = 0;
            while(inp[i] != ' ' && inp[i] != '\t' && inp[i] != 0 && inp[i] != '\n' && inp[i] != ';'){
                buff[j++] = inp[i];
                if(inp[i++] == '>')
                    a->redirect_err = 1;
            }
            a->redirect = strdup(buff);
            memset(buff, 0, 514);

            while(inp[i] != ';' && inp[i] != '\n'){
                if((inp[i] == ' ' || inp[i] == '\t') && inp[i] != '>')
                    i++;
                else if(inp[i] != ';' && inp[i] != '\n'){
                    i++;
                    a->redirect_err = 1;
                }
                
            }
            
            if(inp[i] == ';'){
                    i++;
                    a->next = (struct cmd *)malloc(sizeof(struct cmd));
                    a = a->next;
                    a->cmd_name = NULL;
                    a->cmd_args = NULL;
                    a->redirect = NULL;
                    a->next = NULL;
                    a->redirect_err = 0;
            }

    }

    return out;
}

char **arglistarr(struct cmd *a, int* count)
{
    struct cmd_args *curr = a->cmd_args;
    while(curr != NULL){
        (*count)++;
        curr = curr->next;
    }

    char **arr = malloc(((*count) + 2)*sizeof(char *));
    arr[0] = a->cmd_name;

    curr = a->cmd_args;
    int i = 1;
    while(curr != NULL){
        (*count)++;
        arr[i++] = curr->val;
        curr = curr->next;
    }

    arr[i] = NULL;
    (*count)++;
    return arr;
}

void toolongcmd(char *msg, FILE *fd)
{
    if(fd == NULL){
        write(STDOUT_FILENO, msg, strlen(msg));
        char tmp[2] = {0, 0};
        char c = getc(stdin);
        while(c != '\n'){
            tmp[0] = c;
            write(STDOUT_FILENO, tmp, 1);
            c = getc(stdin);
        }
        tmp[0] = '\n';
        write(STDOUT_FILENO, tmp, 1);
        myError();
    } else {
        write(STDOUT_FILENO, msg, strlen(msg));
        char tmp[2] = {0, 0};
        char c = getc(fd);
        while(c != '\n'){
            tmp[0] = c;
            write(STDOUT_FILENO, tmp, 1);
            c = getc(fd);
        }
        tmp[0] = '\n';
        write(STDOUT_FILENO, tmp, 1);
        myError();
    }
}