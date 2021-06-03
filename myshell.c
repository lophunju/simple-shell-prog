/**
 *  2021-1 / System Programming / Simple Shell Program / B511072 / 박주훈(Park Juhun)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX 256
#define CNT 200

typedef struct pipeList{
    int fd[2];
} PList;

char buf[] = "$ ";
char command[MAX+1];
char lcommand[MAX+1];
char prog[MAX+1];
int cnt;
int lcnt;


/* debugging modules */
void showTokens(char* argvec[]){

    int i;
    for(i=0; argvec[i] != NULL; i++){
        printf("%s\n", argvec[i]);
        fflush(stdout);
    }
}

/* program modules */
void printPrompt(){

    if ( write(STDOUT_FILENO, buf, sizeof(buf)-1) != sizeof(buf)-1){
        perror("write error");
        exit(1);
    }
}

int getCommand(){
    // Ctrl D (EOF) 처리 필요
    int isEOF;

    if (fgets(command, MAX, stdin) == NULL){
        isEOF = 1;
    } else {
        command[strlen(command)-1] = '\0';    
        isEOF = 0;
    }
    
    return isEOF;
}

void tokenize(char* argvec[]){
    
    /* count token amount */
    cnt = 0;

    // save command
    char save[MAX+1];
    strcpy(save, command);

    char* temp;
    temp = strtok(command, "|");
    
    while (temp != NULL){
        cnt++;
        temp = strtok(NULL, "|");
    }
    /* count done */

    // restore command
    strcpy(command, save);

    /* tokenize */
    temp = strtok(command, "|");
    char* token = (char*)malloc(sizeof(char) * (strlen(temp)+1) );
    strcpy(token, temp);
    argvec[0] = token;

    int i;
    for(i=1; i<cnt; i++){
        temp = strtok(NULL, "|");
        token = (char*)malloc(sizeof(char) * (strlen(temp)+1) );
        strcpy(token, temp);
        argvec[i] = token;
    }

    argvec[cnt] = NULL; // terminated by null pointer
    /* tokenize done */

    // save program name
    // strcpy(prog, argvec[0]);
}

void localTokenize(char* argvec[]){
    
    /* count token amount */
    lcnt = 0;

    // save lcommand
    char save[MAX+1];
    strcpy(save, lcommand);

    char* temp;
    temp = strtok(lcommand, " ");
    
    while (temp != NULL){
        lcnt++;
        temp = strtok(NULL, " ");
    }
    /* count done */

    // restore lcommand
    strcpy(lcommand, save);

    /* tokenize */
    temp = strtok(lcommand, " ");
    char* token = (char*)malloc(sizeof(char) * (strlen(temp)+1) );
    strcpy(token, temp);
    argvec[0] = token;

    int i;
    for(i=1; i<lcnt; i++){
        temp = strtok(NULL, " ");
        token = (char*)malloc(sizeof(char) * (strlen(temp)+1) );
        strcpy(token, temp);
        argvec[i] = token;
    }

    argvec[lcnt] = NULL; // terminated by null pointer
    /* tokenize done */

    // save program name
    strcpy(prog, argvec[0]);
}

void freeMem(char* argvec[]){
    int i;
    for(i=0; i<cnt; i++){
        free(argvec[i]);
    }
}

void modifyFirstArg(char* argvec[]){
    char* ptr;
    ptr = strrchr(argvec[0], '/');
    int index = ptr - argvec[0];
    int len = strlen(argvec[0]) - index + 1;

    char temp[MAX+1];
    strncpy(temp, argvec[0]+index+1, len);
    strcpy(argvec[0], temp);
}

void redirectIO(int flag, char* argvec[], int idx){
    
    if (flag){
        switch (flag){  // redirect I/O
        int fd;
        case 1:
            fd = open(argvec[idx+1], O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);
            if (fd < 0){
                perror("open error0");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0){
                perror("dup error");
                exit(1);
            }
            if (close(fd) < 0){
                perror("close error");
                exit(1);
            }
            break;
        case 2:
            fd = open(argvec[idx+1], O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
            if (fd < 0){
                perror("open error1");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0){
                perror("dup error");
                exit(1);
            }
            if (close(fd) < 0){
                perror("close error");
                exit(1);
            }
            break;
        case 3:
            fd = open(argvec[idx+1], O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
            if (fd < 0){
                perror("open error2");
                exit(1);
            }
            if (dup2(fd, STDERR_FILENO) < 0){
                perror("dup error");
                exit(1);
            }
            if (close(fd) < 0){
                perror("close error");
                exit(1);
            }
            break;
        }

        argvec[idx] = NULL; // terminate
    }

}

int main(int argc, char** argv) {

    pid_t pid;

    while(1) {
        printPrompt();
        int isEOF = getCommand();
        if (isEOF){ // ctrl-D
            return 0;
        }

        if (command[0] == '\0')
            continue;

        int isBG;
        if ( command[strlen(command)-1] == '&'){
            isBG = 1;
            command[strlen(command)-1] = '\0';
        }
        else isBG = 0;


        char* argvec[CNT];
        
        tokenize(argvec);

        // printf("global tokens:\n");
        // showTokens(argvec);

        // printf("cnt: %d\n", cnt);

        

        PList plist[cnt-1];
        int pcnt=0;


        int i;
        for (i=0; i<cnt; i++){

            char* largvec[CNT];
            strcpy(lcommand, argvec[i]);
            localTokenize(largvec);
            // printf("local tokens:\n");
            // showTokens(largvec); // for debugging


            // execvp or execv?
            int usep;
            if (prog[0] == '/' | prog[0] == '.') usep = 0;
            else usep = 1;

            // if use execv -> modify first argument
            if (usep == 0){
                modifyFirstArg(largvec);
                // printf("modified: %s\n", argvec[0]);
            }

            // printf("cnt: %d\n", cnt);
            if ( !(i==0 && cnt==1) && !(i!=0 && i==cnt-1) ){    // 명령어에 파이프가 없거나 마지막 파이프 이후의 명령어면 실행 안함
                if (pipe(plist[i].fd) < 0){   // pipe after myself
                    perror("pipe error");
                    exit(1);
                }
                pcnt++;
            }

            // printf("let's fork!\n");
            if( (pid = fork()) < 0){
                perror("fork error");
                exit(1);
            } else if (pid == 0){   // child
                // printf("i'm child. let's exec!\n");


                // I/O redirection 처리
                int flag=0;
                int j;
                for (j=0; j<lcnt; j++){  // every I/O redirection
                    if ( strcmp(largvec[j], "<") == 0) {flag = 1;}
                    if ( strcmp(largvec[j], ">") == 0) {flag = 2;}
                    if ( strcmp(largvec[j], "2>") == 0) {flag = 3;}

                    if (flag){
                        redirectIO(flag, largvec, j);
                    }
                    flag = 0;
                }

                // printf("i=%d, cnt=%d\n", i, cnt);
                
                // pipe 처리
                // if i==0 && cnt==1 (no pipe)
                if (i==0 && cnt==1){
                    // do nothing with pipe
                } else if (i==0 && cnt>1) { // if i==0 && cnt>1 (first)
                    // printf("first\n");
                    // fflush(stdout);
                    if (dup2(plist[i].fd[1], STDOUT_FILENO) < 0){    // pipe after myself
                        perror("dup1 error");
                        exit(1);
                    }

                    int k;
                    for(k=0; k<pcnt; k++){   // parent close
                        if (close(plist[k].fd[0]) < 0){
                            perror("close11 error");
                            exit(1);
                        }
                        if (close(plist[k].fd[1]) < 0){
                            perror("close12 error");
                            exit(1);
                        }
                        // printf("i do it\n");
                    }

                    // if (close(plist[i].fd[1]) < 0){
                    //     perror("close1 error");
                    //     exit(1);
                    // }
                    // if (close(plist[i].fd[0]) < 0){
                    //     perror("close2 error");
                    //     exit(1);
                    // }

                } else if (i!=0 && i==cnt-1){   // if i==cnt-1 (last)
                    // printf("last\n");
                    // fflush(stdout);
                    if (dup2(plist[i-1].fd[0], STDIN_FILENO) < 0){    // pipe before myself
                        perror("dup2 error");
                        exit(1);
                    }

                    int k;
                    for(k=0; k<pcnt; k++){   // parent close
                        if (close(plist[k].fd[0]) < 0){
                            perror("close11 error");
                            exit(1);
                        }
                        if (close(plist[k].fd[1]) < 0){
                            perror("close12 error");
                            exit(1);
                        }
                        // printf("i do it\n");
                    }


                    // if (close(plist[i-1].fd[0]) < 0){
                    //     perror("close3 error");
                    //     exit(1);
                    // }
                    // if (close(plist[i-1].fd[1]) < 0){
                    //     perror("close4 error");
                    //     exit(1);
                    // }

                } else {    // (middle)
                    // printf("middle\n");
                    // fflush(stdout);
                    if (dup2(plist[i].fd[1], STDOUT_FILENO) < 0){    // pipe after myself
                        perror("dup1 error");
                        exit(1);
                    }
                    // if (close(plist[i].fd[1]) < 0){
                    //     perror("close1 error");
                    //     exit(1);
                    // }
                    // if (close(plist[i].fd[0]) < 0){
                    //     perror("close2 error");
                    //     exit(1);
                    // }

                    if (dup2(plist[i-1].fd[0], STDIN_FILENO) < 0){    // pipe before myself
                        perror("dup2 error");
                        exit(1);
                    }
                    // if (close(plist[i-1].fd[0]) < 0){
                    //     perror("close3 error");
                    //     exit(1);
                    // }
                    // if (close(plist[i-1].fd[1]) < 0){
                    //     perror("close4 error");
                    //     exit(1);
                    // }

                    int k;
                    for(k=0; k<pcnt; k++){   // parent close
                        if (close(plist[k].fd[0]) < 0){
                            perror("close11 error");
                            exit(1);
                        }
                        if (close(plist[k].fd[1]) < 0){
                            perror("close12 error");
                            exit(1);
                        }
                        // printf("i do it\n");
                    }

                }



                // 세팅 끝났으면 exec
                if (usep == 1){  // if pathname
                    if ( execvp(prog, largvec) < 0){
                        perror("execv error");
                        exit(1);
                    }
                } else { // if filename
                    if ( execv(prog, largvec) < 0){
                        perror("execv error");
                        exit(1);
                    }
                }
                


            } else {    // parent
                // printf("cnt: %d\n", cnt);
                if (i == cnt-1 && cnt > 1){    // 파이프 자식농사 끝
                    // printf("i did it\n");
                    int k;
                    // printf("pcnt: %d\n", pcnt);
                    for(k=0; k<pcnt; k++){   // parent close
                        if (close(plist[k].fd[0]) < 0){
                            perror("close11 error");
                            exit(1);
                        }
                        if (close(plist[k].fd[1]) < 0){
                            perror("close12 error");
                            exit(1);
                        }
                        // printf("i do it\n");
                    }
                }// if close

                if (isBG == 0){ // if not & (background)
                    // printf("i'm parent. let's wait!\n");

                    // pipe면 마지막만 wait, pipe아니면 바로 wait
                    if (cnt > 1){   // pipe
                        if (i == cnt-1){    // last
                            // printf("let's wait\n");
                            if (waitpid(pid, NULL, 0) < 0){
                                perror("waitpid error");
                                exit(1);
                            }
                            // printf("wait done\n");
                        }
                    } else {    // not pipe
                        if (waitpid(pid, NULL, 0) < 0){
                            perror("waitpid error");
                            exit(1);
                        }
                    }

                }// BG if end
                
            }// parent end
         
            
        }// for (one command between pipe) done

        freeMem(argvec);

    }// global while (one prompt) done


    return 0;
}

