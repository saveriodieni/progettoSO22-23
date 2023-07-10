#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_INTERVAL 50
#define MAX_DELAY 5

int is_numeric(char c){
    return c>=48 && c<=57;
}

int main(int argc, char** argv){

    if(argc<2){
        printf("usage ./randomizer <dest>\n");
        exit(EXIT_FAILURE);
    }
    int duration;
    char* filename=argv[1];
    int pid;

    srand(time(0));

    FILE* f=fopen(filename, "w+");

    while(*filename!='\0'){
        if(is_numeric(*filename)){
            pid=atoi(filename);
            break;
        }
        filename++;   
    }

    int arrival_time=rand()%MAX_DELAY;

    if (! f) return -1;

    for(int i=0;i<8;i++){
        if(i==0){
            fprintf(f, "PROCESS %d %d\n",pid,arrival_time);
        }
        else{
            duration=rand()%MAX_INTERVAL+1;
            if(i%2==0){
                fprintf(f, "IO_BURST %d\n", duration);
            }
            else{
                fprintf(f, "CPU_BURST %d\n", duration);
            }
        }
    }
    fclose(f);
}