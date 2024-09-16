#pragma once

//Can include any other headers as needed
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>


typedef struct {
    char *command;
    bool finished;
    bool error; 
    uint64_t arrival_time;   
    uint64_t start_time;
    uint64_t completion_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    uint64_t burst_time;
    bool started; 
    int process_id;

} Process;



// Function prototypes
void ShortestJobFirst();
void ShortestRemainingTimeFirst();
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime);


 


uint64_t timeinms(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t ms=(tv.tv_sec*1000)+(tv.tv_usec/1000);
    return ms;
}

void CreateFile(char *name){
    FILE *file=fopen(name,"a");
    fprintf(file,"%s,%s,%s,%s,%s,%s,%s\n","Command","Finished","Error","Burst Time","Turnaround Time","Waiting Time","Response Time");
    fclose(file);
}

void insert(Process pr,char *name){
    FILE *file=fopen(name,"a");
    fprintf(file,"\"%s\",%s,%s,%ld,%ld,%ld,%ld\n",pr.command,pr.finished ? "Yes":"No",pr.error ? "Yes":"No",pr.burst_time,pr.turnaround_time,pr.waiting_time,pr.response_time);
    fclose(file);
}


typedef struct Node{
    int index;
    struct Node *next;
} Node;

typedef struct Queue{
    Node *FirstNode;
    Node *LastNode;
} Queue;

Node* newNode(int i){
    Node *temp=(Node*)malloc(sizeof(Node));
    temp->index=i;
    temp->next=NULL;
    return temp;
}

Queue* createQueue(){
    Queue *q=(Queue*) malloc(sizeof(Queue));
    q->FirstNode=NULL;
    q->LastNode=NULL;
    return q;
}

void insertQueue(Queue *Q,Node *node){
    if(Q->FirstNode==NULL){
        Q->FirstNode=node;
        Q->LastNode=node;
        Q->LastNode->next=NULL;
    }
    else{
        Q->LastNode->next=node;
        Q->LastNode=node;
        Q->LastNode->next=NULL;
    }
}

void printQueue(Queue *Q){
    Node *curr=Q->FirstNode;
    if(curr==NULL){
        printf("empty Queue\n");
    }
    while(curr!=NULL){
        printf("current node is %d\n",curr->index);
        curr=curr->next;
    }
}

void RemoveNode(Queue *Q){
    if(Q->FirstNode!=NULL){
        Node *temp=Q->FirstNode;
        Q->FirstNode=Q->FirstNode->next;
        if(Q->FirstNode==NULL){
            Q->LastNode=NULL;
        }
        free(temp);
    }
}

void UpgradeAllNode(Queue *Q1,Queue *Q2,Queue *Q3){
    if(Q1->FirstNode!=NULL){
        Q1->LastNode->next=Q2->FirstNode;
        if(Q2->LastNode!=NULL){
            Q1->LastNode=Q2->LastNode;
        }
    }
    else{
        Q1->FirstNode=Q2->FirstNode;
        Q1->LastNode=Q2->LastNode;
    }

    if(Q1->FirstNode!=NULL){
        Q1->LastNode->next=Q3->FirstNode;
        if(Q3->LastNode!=NULL){
            Q1->LastNode=Q3->LastNode;
        }
    }
    else{
        Q1->FirstNode=Q3->FirstNode;
        Q1->LastNode=Q3->LastNode;
    }
    Q2->FirstNode=NULL;
    Q3->FirstNode=NULL;
    Q2->LastNode=NULL;
    Q3->LastNode=NULL;
}

typedef struct storage{
    char *cmd;
    int tr;
    uint64_t avgbt;
} storage;

storage map[50];
int count=0;

void taking_input(char *cmds){
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds); 
    timeout.tv_sec=0;
    timeout.tv_usec=0;
    int ready=select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

    if (ready==-1){
        exit(-1);
    } 
    else if(ready==0){
        ;
    }
    else{
        if(FD_ISSET(STDIN_FILENO,&read_fds)){
            char temp[1024];
            while (fgets(temp, 1024, stdin)) {
                strcat(cmds, temp);  
                if (strcmp(temp, "\n") == 0) {
                    break;
                }
            }
        }
    }
}

void MultiLevelFeedbackQueue(int quantum0,int quantum1,int quantum2,int boostTime){
    int fp=0; //count of finished processes
    Process p[50];
    Queue* Q1=createQueue();
    Queue* Q2=createQueue();
    Queue* Q3=createQueue();
    char cmds[1024];
    memset(cmds, 0, sizeof(cmds));
    taking_input(cmds);
    char *token = strtok(cmds,"\n");
    while ( token!=NULL) {  
        p[count].command=strdup(token);
        Node *temp=newNode(count);
        insertQueue(Q2,temp);
        p[count].finished=false;
        p[count].started=false;
        p[count].error=false;
        p[count].burst_time=0;
        p[count].arrival_time=0;
        count++;
        token = strtok(NULL, "\n");
    }

    uint64_t start=timeinms();
    char *filename="result_online_MLFQ.csv";
    CreateFile(filename);
    uint64_t conttime;
    uint64_t past=0;
    uint64_t present=0;
    while(1){
        if(Q1->FirstNode!=NULL){
            int i=Q1->FirstNode->index;
            if(p[i].started){
                conttime=timeinms()-start;
                kill(p[i].process_id,SIGCONT);
            }
            else{
                int r=fork();
                if(r<0){
                    fprintf(stderr,"fork failed\n");
                }
                else if(r==0){
                    char *input=strdup(p[i].command);
                    char *myargs[512 + 1];
                    char *token1 = strtok(input, " ");
                    int j=0;
                    while (token1 != NULL) {  
                        myargs[j] = token1;
                        j++;
                        token1 = strtok(NULL, " ");
                    }
                    myargs[j] = NULL;
                    execvp(myargs[0],myargs);
                    exit(1);
                }
                else{
                    conttime=timeinms()-start;
                    p[i].start_time=conttime;
                    p[i].response_time=p[i].start_time-p[i].arrival_time;
                    p[i].started=true;
                    p[i].process_id=r;
                } 
            }
            uint64_t aft;
            int status;
            int res;
            if(fp==count-1){
                res=waitpid(p[i].process_id,&status,0);
            }
            else{
                usleep(quantum0*1000);
                res=waitpid(p[i].process_id,&status,WNOHANG);
            }

            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
                Node* temp=newNode(i);
                insertQueue(Q2,temp);
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=false;
                aft=timeinms()-start;
                insert(p[i],filename);
            }
            else{
                aft=timeinms()-start;
                p[i].error=false;
                p[i].finished=true;
                p[i].completion_time=aft;
                p[i].turnaround_time=p[i].completion_time-p[i].arrival_time;
                p[i].burst_time+=aft-conttime;
                p[i].waiting_time=p[i].turnaround_time-p[i].burst_time;
                insert(p[i],filename);
                bool flag=false;
                for(int j=0;j<fp;j++){
                    if(strcmp(map[j].cmd,p[i].command)==0){
                        map[j].avgbt=(map[j].tr*map[j].avgbt+ p[i].burst_time)/(map[j].tr+1);
                        map[j].tr++;
                        flag=true;
                        break;
                    }
                }
                if(!flag){
                    map[fp].avgbt=p[i].burst_time;
                    map[fp].cmd=strdup(p[i].command);
                    map[fp].tr=1;
                }
                fp++;
            }
            RemoveNode(Q1);
            printf("In Priority 1 Queue | Command: %s | start_time of context: %ld | completion_time of context: %ld\n",p[i].command,conttime,aft);

        }
        else if(Q2->FirstNode!=NULL){
            int i=Q2->FirstNode->index;

            if(p[i].started){
                conttime=timeinms()-start;
                kill(p[i].process_id,SIGCONT);
            }
            else{
                int r=fork();
                if(r<0){
                    fprintf(stderr,"fork failed\n");
                }
                else if(r==0){
                    char *input=strdup(p[i].command);
                    char *myargs[512 + 1];
                    char *token2 = strtok(input, " ");
                    int j=0;
                    while (token2 != NULL) {  
                        myargs[j] = token2;
                        j++;
                        token2 = strtok(NULL, " ");
                    }
                    myargs[j] = NULL;
                    execvp(myargs[0],myargs);
                    exit(1);
                }
                else{
                    conttime=timeinms()-start;
                    p[i].start_time=conttime;
                    p[i].response_time=p[i].start_time-p[i].arrival_time;
                    p[i].started=true;
                    p[i].process_id=r;
                } 
            }
            uint64_t aft;
            int status;
            int res;
            if(fp==count-1){
                res=waitpid(p[i].process_id,&status,0);
            }
            else{
                usleep(quantum1*1000);
                res=waitpid(p[i].process_id,&status,WNOHANG);
            }

            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
                Node* temp=newNode(i);
                insertQueue(Q3,temp);
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=false;
                aft=timeinms()-start;
                insert(p[i],filename);
            }
            else{
                aft=timeinms()-start;
                p[i].error=false;
                p[i].finished=true;
                p[i].completion_time=aft;
                p[i].turnaround_time=p[i].completion_time-p[i].arrival_time;
                p[i].burst_time+=aft-conttime;
                p[i].waiting_time=p[i].turnaround_time-p[i].burst_time;
                insert(p[i],filename);
                bool flag=false;
                for(int j=0;j<fp;j++){
                    if(strcmp(map[j].cmd,p[i].command)==0){
                        map[j].avgbt=(map[j].tr*map[j].avgbt+ p[i].burst_time)/(map[j].tr+1);
                        map[j].tr++;
                        flag=true;
                        break;
                    }
                }
                if(!flag){
                    map[fp].avgbt=p[i].burst_time;
                    map[fp].cmd=strdup(p[i].command);
                    map[fp].tr=1;
                }
                fp++;
            }
            RemoveNode(Q2);
            printf("In Priority 2 Queue | Command: %s | start_time of context: %ld | completion_time of context: %ld\n",p[i].command,conttime,aft);
            
        }
        else if(Q3->FirstNode!=NULL){

            int i=Q3->FirstNode->index;  
            if(p[i].started){
                conttime=timeinms()-start;
                kill(p[i].process_id,SIGCONT);
            }
            else{
                int r=fork();
                if(r<0){
                    fprintf(stderr,"fork failed\n");
                }
                else if(r==0){
                    char *input=strdup(p[i].command);
                    char *myargs[512 + 1];
                    char *token3 = strtok(input, " ");
                    int j=0;
                    while (token3 != NULL) {  
                        myargs[j] = token3;
                        j++;
                        token3 = strtok(NULL, " ");
                    }
                    myargs[j] = NULL;
                    execvp(myargs[0],myargs);
                    exit(1);
                }
                else{
                    conttime=timeinms()-start;
                    p[i].start_time=conttime;
                    p[i].response_time=p[i].start_time-p[i].arrival_time;
                    p[i].started=true;
                    p[i].process_id=r;
                } 
            }

            uint64_t aft;
            int status;
            int res;
            if(fp==count-1){
                res=waitpid(p[i].process_id,&status,0);
            }
            else{
                usleep(quantum2*1000);
                res=waitpid(p[i].process_id,&status,WNOHANG);
            }

            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
                Node* temp=newNode(i);
                insertQueue(Q3,temp);
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=false;
                aft=timeinms()-start;
                insert(p[i],filename);
            }
            else{
                aft=timeinms()-start;
                p[i].error=false;
                p[i].finished=true;
                p[i].completion_time=aft;
                p[i].turnaround_time=p[i].completion_time-p[i].arrival_time;
                p[i].burst_time+=aft-conttime;
                p[i].waiting_time=p[i].turnaround_time-p[i].burst_time;
                insert(p[i],filename);
                bool flag=false;
                for(int j=0;j<fp;j++){
                    if(strcmp(map[j].cmd,p[i].command)==0){
                        map[j].avgbt=(map[j].tr*map[j].avgbt+ p[i].burst_time)/(map[j].tr+1);
                        map[j].tr++;
                        flag=true;
                        break;
                    }
                }
                if(!flag){
                    map[fp].avgbt=p[i].burst_time;
                    map[fp].cmd=strdup(p[i].command);
                    map[fp].tr=1;
                }
                fp++;
            }
            RemoveNode(Q3);
            printf("In Priority 3 Queue | Command: %s | start_time of context: %ld | completion_time of context: %ld\n",p[i].command,conttime,aft);
        }
        
        present=timeinms()-start;
        if(present-past>=boostTime){
            UpgradeAllNode(Q1,Q2,Q3);
            past=timeinms()-start;
        }
        memset(cmds, 0, sizeof(cmds));
        taking_input(cmds);
        char *token4 = strtok(cmds,"\n");
        uint64_t itm=timeinms()-start;
        while (token4 != NULL && strlen(token4)>0) {  
            p[count].command=strdup(token4);
            Node *temp=newNode(count);
            bool flag=false;
            for(int j=0;j<fp;j++){
                if(strcmp(token4,map[j].cmd)==0){
                    if(map[j].avgbt<=quantum0){
                        insertQueue(Q1,temp);
                    }
                    else if(map[j].avgbt <=quantum1){
                        insertQueue(Q2,temp);
                    }
                    else{
                        insertQueue(Q3,temp);
                    }
                    flag=true;
                    break;
                }
            }
            if(!flag){
                insertQueue(Q2,temp);
            }
            p[count].finished=false;
            p[count].started=false;
            p[count].error=false;
            p[count].burst_time=0;
            p[count].arrival_time=itm;
            count++;
            token4 = strtok(NULL, "\n");
            
        }

    }
}
