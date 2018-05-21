#include "types.h"
#include "user.h"
#include "fcntl.h"

#define CHILD_NUM 2
#define PGSIZE 4096
#define MEM_SIZE 10000
#define SZ 1200

void child_test(){
    if(!fork()){
        int children[CHILD_NUM];
        int* arr[CHILD_NUM];
        int ind=0;
        for(int i=0;i<CHILD_NUM;++i){
            arr[i] = (int*)(malloc(MEM_SIZE*sizeof(int)));
            children[i] = fork();
            ind = children[i]?ind:i+1;
            for(int j=0;j<MEM_SIZE;++j){
                arr[i][j] = children[i];
            }
        }
        for(int i=ind;i<CHILD_NUM;++i){
        wait();
        }
        for(int i=0;i<CHILD_NUM;++i){
            int sum = 0;
            for(int j=0;j<MEM_SIZE;++j){
                sum = sum + arr[i][j];
            }
            sum = sum/MEM_SIZE;
            free(arr[i]);
            printf(2,"arr[%d] avg = %d\n",i,sum);
        }
        exit();
    }else{
        wait();
    }
}

void alloc_dealloc_test(){
    if(!fork()){
        int* arr = (int*)(sbrk(PGSIZE*20));
        for(int i=0;i<PGSIZE*20/sizeof(int);++i){arr[i]=0;}
        sbrk(-PGSIZE*20);
        printf(2,"dealloc complete\n");
        arr = (int*)(sbrk(PGSIZE*20));
        for(int i=0;i<PGSIZE*20/sizeof(int);++i){arr[i]=2;}
        for(int i=0;i<PGSIZE*20/sizeof(int);++i){printf(2,"arr[%d]=%d\n",i,arr[i]);}
        sbrk(-PGSIZE*20);
        exit();
    }else{
        wait();
    }
}

void advance_alloc_dealloc_test(){
    if(!fork()){
        int* arr = (int*)(sbrk(PGSIZE*20));
        for(int i=0;i<PGSIZE*20/sizeof(int);++i){arr[i]=0;}
        int pid=fork();
        if(!pid){
            sbrk(-PGSIZE*20);
            printf(2,"dealloc complete\n");
            printf(2,"should cause segmentation fault\n");
            for(int i=0;i<PGSIZE*20/sizeof(int);++i){
            arr[i]=1;
            }
            printf(2,"test failed\n");
            exit();
        }
        wait();
        int sum=0;
        for(int i=0;i<PGSIZE*20/sizeof(int);++i){sum=sum+arr[i];}
        sbrk(-PGSIZE*20);
        int father=1;
        char* bytes;
        char* origStart = sbrk(0);
        int count=0;
        int max_size=0;
        for(int i=0;i<20;++i){
            father = father && fork()>0;
            if(!father){break;}
            if(father){
                bytes = (char*)(sbrk(PGSIZE));
                max_size = max_size+PGSIZE;
                for(int i=0;i<PGSIZE;++i){bytes[i]=1;}
            }
        }
        for(int i=0;i<max_size;++i){
            count = count + origStart[i];
        }
        printf(2,"count:%d\n",count);
        if(father){
            for(int i=0;i<20;++i){wait();}
        }
        exit();
    }else{
        wait();
    }
}

void exec_test(){
    printf(2,"exec test\n");
    printf(2,"allocating pages\n");
    int* arr = (int*)(malloc(sizeof(int)*5*PGSIZE));
    for(int i=0;i<5*PGSIZE;i=i+PGSIZE){
      arr[i]=i/PGSIZE;
    }
    printf(2,"forking\n");
    int pid = fork();
    if(!pid){
        char* argv[] = {"myMemTest","exectest",0};
        exec(argv[0],argv);
    }else{
        wait();
    }
}

void exec_test_child(){
    printf(2,"child allocating pages\n");
    int* arr = (int*)(malloc(sizeof(int)*5*PGSIZE));
    for(int i=0;i<5*PGSIZE;i=i+PGSIZE){
      arr[i]=i/PGSIZE;
    }
    printf(2,"child exiting\n");
}

int main(int argc, char** argv){
    if(argc>=1){
      if(strcmp(argv[1],"exectest")==0){
        exec_test_child();
        exit();
      }
    }
    exec_test();
    alloc_dealloc_test();
    advance_alloc_dealloc_test();
    child_test();
    exit();
}

