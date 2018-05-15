#include "types.h"
#include "user.h"
#include "fcntl.h"

#define SZ 120

int main(int argc, char** argv){
    int** arr = (int**)(malloc(SZ*sizeof(int*)));
    for(int i=0;i<SZ;++i){
      arr[i] = (int*)(malloc(SZ*sizeof(int)));
      for(int j=0;j<SZ;++j){
        arr[i][j] = i+j;
      }
    }
    for(int i=0;i<SZ;++i){
      for(int j=0;j<SZ;++j){
        printf(2,"%d ",arr[i][j]);
      }
      printf(2,"\n");
    }
    exit();
}

