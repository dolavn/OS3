#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

#define FREE_SLOT   1

#define OFFSET(ind) (ind*PGSIZE)

void printbits(uint* addr){
  uint bits = *addr;
  char output[33]; output[32]=0;
  for(int i=31;i>=0;--i){
    output[i] = bits%2==0?'0':'1';
    bits /= 2;
  }
  cprintf("%s\n",output);
}

int find_page_ind(struct proc* p,uint* page){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i] == page){return i;}
  }
  return -1;
}

int init_page_meta(struct proc* p){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    p->pages[i] = (uint*)(FREE_SLOT);
  }
  return 0;
}

int find_free_slot(){
  struct proc* p = myproc();
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i]==(uint*)(FREE_SLOT)){return i;}
  }
  panic("no free slots");
}

int swap_to_file(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  char* addr = (char*)(PTE_ADDR(*page));
  writeToSwapFile(p,addr,OFFSET(ind),PGSIZE);
  p->pages[ind] = page;
  (*page) &= ~PTE_P;
  (*page) &= PTE_PG;
  return 1;
}

int swap_from_file(uint* page){
  struct proc* p = myproc();
  int ind = -1;
  for(int i=0;i<MAX_TOTAL_PAGES && ind==-1;++i){
    if(page==(uint*)(p->pages[i])){ind = i;}
  }
  if(ind==-1){
    panic("page not found!");
  }
  char* addr = (char*)(PTE_ADDR(*page));
  readFromSwapFile(p,addr,OFFSET(ind),PGSIZE);
  (*page) &= PTE_P;
  (*page) &= ~PTE_PG;
  return 1;
}

int get_page_to_swap(){
  return 0;
  struct proc* p = myproc();
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(*p->pages[i] & PTE_P & ~PTE_PG){return i;}
  }
  return -1;
}

void add_page(struct proc* p,uint* page){
  int ind = find_free_slot();
  p->pages[ind] = page;
  cprintf("ind:%d\n",ind);
  printbits(p->pages[ind]);
}

void remove_page(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  p->pages[ind] = (uint*)(FREE_SLOT);
}
