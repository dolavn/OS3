#include "types.h"
#include "user.h"
#include "fcntl.h"

#define SZ 500

int main(int argc, char** argv){
    int** arr = (int**)(sbrk(SZ*sizeof(int*)));
    for(int i=0;i<SZ;++i){
      arr[i] = (int*)(sbrk(SZ*sizeof(int)));
      printf(2,"addr:%p\n",arr[i]);
    }
    printf(2,"%p\n",arr);
    exit();
}

