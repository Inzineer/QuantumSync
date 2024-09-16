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


typedef struct {
    char *command;
    bool finished;
    bool error;    
    uint64_t start_time;
    uint64_t completion_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    bool started; 
    int process_id;

} Process;



// Function prototypes
void ShortestJobFirst();
void ShortestRemainingTimeFirst();
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime);

int count=0;
void process_command(char *command){
    count++;
    
}
void input() {
    fd_set read_fds;
    struct timeval timeout;
    char buffer[BUFFER_SIZE];
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
            fgets(buffer, sizeof(buffer), stdin);
            char *command = strtok(buffer, "\n");
            while (command != NULL) {
                process_command(command);
                command = strtok(NULL, "\n");
            }
        }
    }
}
 


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

void ShortestJobFirst(){

}



void MultiLevelFeedbackQueue(int quantum0,int quantum1,int quantum2,int boostTime){
    Process p[50];
    struct Queue* Q1=createQueue();
    struct Queue* Q2=createQueue();
    struct Queue* Q3=createQueue();
    input();
    uint64_t start=timeinms();
    char *filename="result_online_MLFQ.csv";
    CreateFile(filename);
    int count=0;
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
                    char *token = strtok(input, " ");
                    int j=0;
                    while (token != NULL) {  
                        myargs[j] = token;
                        j++;
                        token = strtok(NULL, " ");
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
            usleep(quantum0*1000);
            int status;
            int res=waitpid(p[i].process_id,&status,WNOHANG);
            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
                struct Node* temp=newNode(i);
                insertQueue(Q2,temp);
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=true;
                aft=timeinms()-start;
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
                count++;
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
                    char *token = strtok(input, " ");
                    int j=0;
                    while (token != NULL) {  
                        myargs[j] = token;
                        j++;
                        token = strtok(NULL, " ");
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
            usleep(quantum1*1000);
            int status;
            int res=waitpid(p[i].process_id,&status,WNOHANG);
            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
                struct Node* temp=newNode(i);
                insertQueue(Q3,temp);
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=true;
                aft=timeinms()-start;
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
                count++;
            }
            RemoveNode(Q2);
            printf("In Priority 2 Queue | Command: %s | start_time of context: %ld | completion_time of context: %ld\n",p[i].command,conttime,aft);
            present=timeinms()-start;
            if(present-past>=boostTime){
                UpgradeAllNode(Q1,Q2,Q3);
                past=timeinms()-start;
            }
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
                    char *token = strtok(input, " ");
                    int j=0;
                    while (token != NULL) {  
                        myargs[j] = token;
                        j++;
                        token = strtok(NULL, " ");
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
            usleep(quantum2*1000);
            int status;
            int res=waitpid(p[i].process_id,&status,WNOHANG);
            if(res==0){
                kill(p[i].process_id,SIGSTOP);
                aft=timeinms()-start;
                p[i].burst_time+=aft-conttime;
            } 
            else if(!WIFEXITED(status) || WEXITSTATUS(status)!=0){
                p[i].error=true;
                p[i].finished=true;
                aft=timeinms()-start;
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
                count++;
                RemoveNode(Q3);
            }
            printf("In Priority 3 Queue | Command: %s | start_time of context: %ld | completion_time of context: %ld\n",p[i].command,conttime,aft);
            present=timeinms()-start;
            if(present-past>=boostTime){
                UpgradeAllNode(Q1,Q2,Q3);
                past=timeinms()-start;
            }
        }
        input();
    }
}
