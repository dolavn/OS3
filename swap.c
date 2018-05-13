#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

#define FREE_SLOT   1
#define TAKEN_OFFSET -1

void printbits(uint* addr){
  uint bits = *addr;
  char output[33]; output[32]=0;
  for(int i=31;i>=0;--i){
    output[i] = bits%2==0?'0':'1';
    bits /= 2;
  }
  cprintf("%s\n",output);
}

int get_offset(struct proc* p){
  for(int i=0;i<MAX_SWAP_FILE_SZ;++i){
    if(p->offsets[i]!=TAKEN_OFFSET){
      int ans = p->offsets[i];
      p->offsets[i] = TAKEN_OFFSET;
      return ans;
    }
  }
  panic("swap file");
}

void free_offset(struct proc* p,int offset){
  for(int i=0;i<MAX_SWAP_FILE_SZ;++i){
    if(p->offsets[i]==TAKEN_OFFSET){
      p->offsets[i] = offset;
      return;
    }
  }
  panic("swap file");
}

int find_page_ind(struct proc* p,uint* page){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i].pte == page){return i;}
  }
  return -1;
}

int init_page_meta(struct proc* p){
  p->num_of_pages = 0;
  p->phys_pages = 0;
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    p->pages[i].pte = (uint*)(FREE_SLOT);
    p->pages[i].taken = 0;
    p->pages[i].offset = 0;
    p->pages[i].on_phys = 0;
  }
  for(int i=0;i<MAX_SWAP_FILE_SZ;++i){
    p->offsets[i] = i*PGSIZE;
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
  int offset = get_offset(p);
  char* pa = (char*)(PTE_ADDR(*page));
  char* va = (char*)(P2V((uint)(pa)));
  writeToSwapFile(p,va,offset,PGSIZE);
  p->pages[ind].pte = page;
  p->pages[ind].offset = offset;
  p->pages[ind].on_phys = 0;
  if(offset+PGSIZE>p->file_size){
    p->file_size = offset+PGSIZE;
  }
  (*page) &= ~PTE_P;
  (*page) |= PTE_PG;
  p->phys_pages--;
  return 1;
}

int swap_from_file(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  char* addr = (char*)(PTE_ADDR(*page));
  int offset = p->pages[ind].offset;
  readFromSwapFile(p,P2V(addr),offset,PGSIZE);
  free_offset(p,offset);
  (*page) |= PTE_P;
  (*page) &= ~PTE_PG;
  p->pages[ind].on_phys = 1;
#ifdef NFUA
  p->pages[ind].counter = 0;
#endif
#ifdef LAPA
  p->pages[ind].counter = -1; //0xFFFFFFFF
#endif
  p->phys_pages++;
  return 1;
}

int get_page_to_swap(){
  struct proc* p = myproc();
  for(int i=6;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i].on_phys){return i;}
  }
  return -1;
}

void add_page(struct proc* p,uint* page){
  int ind = find_free_slot();
  p->pages[ind].pte = page;
  p->pages[ind].taken = 1;
  p->pages[ind].on_phys = 1;
  p->num_of_pages++;
#ifdef NFUA
  p->pages[ind].counter = 0;
#endif
#ifdef LAPA
  p->pages[ind].counter = -1; //0xFFFFFFFF
#endif
  p->phys_pages++;
  cprintf("phys:%d\ntotal:%d\n",p->phys_pages,p->num_of_pages);
}

void remove_page(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  p->pages[ind].taken = 0;
}

void copy_page_arr(struct proc* dst,struct proc* src){
  dst->num_of_pages = src->num_of_pages;
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    dst->pages[i] = src->pages[i];
  }
}

void copy_swap_file(struct proc* dst, struct proc* src) {
  int file_size = src->file_size;
  char buf[file_size];
  readFromSwapFile(src, buf, 0, file_size);
  writeToSwapFile(dst, buf, 0, file_size);
  dst->file_size = file_size;
}
