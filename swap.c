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
    if(p->pages[i].pte == page){return i;}
  }
  return -1;
}

int init_page_meta(struct proc* p){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    p->pages[i].pte = (uint*)(FREE_SLOT);
    p->pages[i].taken = 0;
    p->pages[i].offset = 0;
    p->pages[i].on_phys = 0;
  }
  return 0;
}

int find_free_slot(){
  struct proc* p = myproc();
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(!p->pages[i].taken){return i;}
  }
  panic("no free slots");
}

int swap_to_file(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  int offset = OFFSET(ind);
  char* addr = (char*)(PTE_ADDR(*page));
  writeToSwapFile(p,addr,offset,PGSIZE);
  p->pages[ind].pte = page;
  p->pages[ind].offset = offset;
  p->pages[ind].on_phys = 0;
  (*page) &= ~PTE_P;
  (*page) &= PTE_PG;
  return 1;
}

int swap_from_file(uint* page){
  struct proc* p = myproc();
  int ind = -1;
  for(int i=0;i<MAX_TOTAL_PAGES && ind==-1;++i){
    if(page==(uint*)(p->pages[i].pte)){ind = i;}
  }
  if(ind==-1){panic("page not found!");}
  char* addr = (char*)(PTE_ADDR(*page));
  readFromSwapFile(p,addr,OFFSET(ind),PGSIZE);
  (*page) &= PTE_P;
  (*page) &= ~PTE_PG;
  p->pages[ind].on_phys = 1;
  return 1;
}

int get_page_to_swap(){
  return 0;
  struct proc* p = myproc();
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i].on_phys){return i;}
  }
  return -1;
}

void add_page(struct proc* p,uint* page){
  int ind = find_free_slot();
  p->pages[ind].pte = page;
  p->pages[ind].taken = 1;
  p->pages[ind].on_phys = 1;
  cprintf("ind:%d\n",ind);
  printbits(p->pages[ind].pte);
}

void remove_page(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  p->pages[ind].taken = 0;
}

void copy_page_arr(struct proc* dst,struct proc* src){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    dst->pages[i] = src->pages[i];
  }
}

void copy_swap_file(struct proc* src, struct proc* dest) {
  int file_size = src.file_size;
  char buf[file_size];
  readFromSwapFile(src, buf, 0, file_size);
  writeToSwapFile(dest, buf, 0, file_size);
}
