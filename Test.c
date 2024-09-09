#include "OfflineSchedulers.h"


int main(int argc, char *argv[]){
    Process p[5];
    p[0].command=strdup("ls -1");
    p[1].command=strdup("pwd");
    p[2].command=strdup("./p1");
    p[4].command=strdup("./p2");
    p[3].command=strdup("invalid command");
    MultiLevelFeedbackQueue(p,5,1000,2000,3000,5000);

    return 0;
}

