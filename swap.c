#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

#define TAKEN_OFFSET -1
#define FREE_SLOT 0

#define min(a,b) (a<b?a:b)

void printbits(uint* addr){
  uint bits = *addr;
  char output[33]; output[32]=0;
  for(int i=31;i>=0;--i){
    output[i] = bits%2==0?'0':'1';
    bits /= 2;
  }
  cprintf("%s\n",output);
}

#ifndef NONE
/*
Returns a new offset in the swap file to which a page can be swapped.
*/
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

/*
Frees an offset in the swap file, that is - mark it available for swapping a page.
*/
void free_offset(struct proc* p,int offset){
  for(int i=0;i<MAX_SWAP_FILE_SZ;++i){
    if(p->offsets[i]==TAKEN_OFFSET){
      p->offsets[i] = offset;
      return;
    }
  }
  panic("swap file");
}

/*
Returns the idnex of the page in the page meta data structure, or -1 if the page
is not in the meta data.
*/
int find_page_ind(struct proc* p,uint* page){
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(p->pages[i].pte == page && p->pages[i].taken){return i;}
  }
  return -1;
}

/*
Returns the index of the next page to be swapped out
*/
int get_page_to_swap() {
  int selected = -1;
#ifdef AQ
  struct proc* p = myproc();
  struct page_meta* page = dequeue(&p->page_queue);
  selected = page-p->pages;
#endif
#if defined(LAPA) || defined(NFUA)
  struct proc* p = myproc();
  struct page_meta* pages = p->pages;
  struct page_meta* currPage;
  uint curr, best = -1;
  for (int i=0; i<MAX_TOTAL_PAGES; i++) {
    currPage = &pages[i];
    if (currPage->taken && currPage->on_phys) {
#ifdef LAPA
      curr = num_of_ones(currPage->counter);
      if (curr < best || (curr == best && currPage->counter < pages[selected].counter)){
        best = curr;
        selected = i;
      }
#endif
#ifdef NFUA
      curr = currPage->counter;
      if (curr < best) {
        best = curr;
        selected = i;
      }
#endif
    }
  }
#endif
#ifdef SCFIFO
  struct proc* p = myproc();
  struct page_meta* pages = p->pages;
  int headInd = p->headp;
  int currInd = p->headp;
  struct page_meta* currPage;
  while (1) {
    currPage = &pages[currInd];
    if (!currPage->on_phys){panic("page on phys");}
    if (*(currPage->pte) & PTE_A) {
      *(currPage->pte) &= ~PTE_A;
      currInd = currPage->nextp;
    }
    else return currInd;
    if (currInd==headInd) return headInd;
  }
  panic("no page chosen");
#endif
  return selected;

}

/*
Copies the swap file of process src, to the swap file of process dst.
*/
void copy_swap_file(struct proc* dst, struct proc* src) {
  int file_size = src->file_size;
  char* buffer = kalloc();
  for(int i=0;i<file_size;i=i+PGSIZE){
    int size = min(file_size-i,PGSIZE);
    readFromSwapFile(src, buffer, i, size);
    writeToSwapFile(dst, buffer, i, size);
  }
  kfree(buffer);
  dst->file_size = file_size;
}

/*
Swaps a given page to the swap file.
*/
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
#ifdef SCFIFO
  p->pages[p->pages[ind].nextp].prevp = p->pages[ind].prevp;
  p->pages[p->pages[ind].prevp].nextp = p->pages[ind].nextp;
  if(p->headp==ind){
    p->headp = ind==p->pages[ind].nextp?-1:p->pages[ind].nextp;
    p->pages[ind].nextp=-1;
    p->pages[ind].prevp=-1;
    
  }
#endif
  return 1;
}

/*
Retrieves a page from the swap file to the main memory.
*/
int swap_from_file(uint* page){
  struct proc* p = myproc();
  int ind = find_page_ind(p,page);
  char* addr = (char*)(PTE_ADDR(*page));
  int offset = p->pages[ind].offset;
  p->pages[ind].offset = -1;
  readFromSwapFile(p,P2V(addr),offset,PGSIZE);
  free_offset(p,offset);
  (*page) |= PTE_P;
  (*page) &= ~PTE_PG;
  p->pages[ind].on_phys = 1;
#ifdef NFUA
  p->pages[ind].counter = 1<<31;
#endif
#ifdef LAPA
  p->pages[ind].counter = -1; //0xFFFFFFFF
#endif

#ifdef SCFIFO
  p->pages[ind].nextp = p->headp; // my next is head (i'm new last)
  p->pages[ind].prevp = p->pages[p->headp].prevp; //my prev is current last = head.prev
  p->pages[p->headp].prevp = ind; // i'm heads new prev
  p->pages[p->pages[ind].prevp].nextp = ind;
#endif
#ifdef AQ
  enqueue(&p->page_queue,&p->pages[ind]);
#endif
  p->phys_pages++;
  return 1;
}

/*
Finds a free index in the page meta data
*/
int find_free_slot(){
  struct proc* p = myproc();
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    if(!p->pages[i].taken){return i;}
  }
  return -1;
}


#endif

/*
Initializes the page meta data
*/
int init_page_meta(struct proc* p){
  p->num_of_pages = 0;
  p->phys_pages = 0;
  p->pgflt_count = 0;
  p->pgout_count = 0;
#ifdef SCFIFO
  p->headp = -1;
#endif
#ifndef NONE
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    p->pages[i].pte = (uint*)(FREE_SLOT);
    p->pages[i].taken = 0;
    p->pages[i].offset = -1;
    p->pages[i].on_phys = 0;
#ifdef SCFIFO
    p->pages[i].nextp=-1;
    p->pages[i].prevp=-1;
#endif
#ifdef AQ
    p->pages[i].queue_location = -1;
#endif
  }
  for(int i=0;i<MAX_SWAP_FILE_SZ;++i){
    p->offsets[i] = i*PGSIZE;
  }
#ifdef AQ
  initQueue(&p->page_queue);
#endif
#endif
  return 0;
}

/*
Adds a page to the page meta data
*/
int add_page(struct proc* p,uint* page,char* va){
  p->num_of_pages++;
  p->phys_pages++;
#ifndef NONE
  if(p->ignorePaging){return 1;}
  int ind = find_free_slot();
  if(ind==-1){
    return 0;
  }
  p->pages[ind].pte = page;
  p->pages[ind].taken = 1;
  p->pages[ind].on_phys = 1;
  p->pages[ind].va = va;
#ifdef NFUA
  p->pages[ind].counter = 0;
#endif
#ifdef LAPA
  p->pages[ind].counter = -1; //0xFFFFFFFF
#endif
#ifdef AQ
  enqueue(&p->page_queue,&p->pages[ind]);
  //cprintf("add page\nphysical pages:%d\nqueue size:%d\n",p->phys_pages,p->page_queue.size);
#endif
#ifdef SCFIFO
  if (p->headp == -1) {
    p->headp = ind;
    p->pages[p->headp].prevp = ind;
    p->pages[p->headp].nextp = ind;
    return 1;
  }
  p->pages[ind].nextp = p->headp; // my next is head (i'm new last)
  p->pages[ind].prevp = p->pages[p->headp].prevp; //my prev is current last = head.prev
  p->pages[p->headp].prevp = ind; // i'm heads new prev
  p->pages[p->pages[ind].prevp].nextp = ind;
#endif
#endif
  return 1;
}

/*
Removes a page from the page meta data
*/
int remove_page(uint* page){
  struct proc* p = myproc();
  if(p->ignorePaging){p->num_of_pages--;p->phys_pages--; return 1;}
#ifndef NONE
  int ind = find_page_ind(p,page);
  if(ind==-1){return 0;}
  p->num_of_pages--;
  if(p->pages[ind].on_phys){p->phys_pages--;}
  p->pages[ind].taken = 0;
  p->pages[ind].pte = 0;
#ifdef SCFIFO
  if(p->pages[ind].on_phys){
    p->pages[p->pages[ind].prevp].nextp = p->pages[ind].nextp;
    p->pages[p->pages[ind].nextp].prevp = p->pages[ind].prevp;
  }
  if(p->headp==ind){
    p->headp=p->pages[ind].nextp==ind?-1:p->pages[ind].nextp;
  }
#endif
#ifdef AQ
  int queue_ind = p->pages[ind].queue_location;
  if(p->pages[ind].on_phys){
    p->pages[ind].queue_location=-1;
    p->page_queue.arr[queue_ind]=0;
    p->page_queue.size--;
  }
#endif
  p->pages[ind].on_phys = 0;
#if defined(NFUA) || defined(LAPA)
  p->pages[ind].counter = 0;
#endif
  if(p->pages[ind].offset!=-1){
    free_offset(p,p->pages[ind].offset);
    p->pages[ind].offset=-1;
  }
#endif
  return 1;
}


#ifdef LAPA
/*
Counts the number of turned on bits in an unsigned int
*/
uint
num_of_ones (uint c) {
  int ans = 0;
  while(c) {
    if (c%2 != 0) ans++;
    c /= 2;
  }
  return ans;
}
#endif

#if defined(NFUA) || defined(LAPA) || defined(AQ)

/*
Updates the page counter in NFUA and LAPA paging policies, and the advancing queue,
in AQ paging policy.
*/
void
updatePagesCounter() {
  struct proc* p = myproc();
  if(p==0){
    return;
  }
  struct page_meta* pages = p->pages;
  struct page_meta* currPage;
#ifndef AQ
  uint adder = 0x80000000;
#endif
  for (int i=0; i<MAX_TOTAL_PAGES; i++) {
    currPage = &pages[i];
    if (currPage->taken && currPage->on_phys){
#ifndef AQ
      currPage->counter >>= 1;
      if ((*(currPage->pte) & PTE_A)){
        currPage->counter |= adder;
        *(currPage->pte) &= ~PTE_A;
      }
#else
      if ((*(currPage->pte) & PTE_A)) {
        advancePage(&p->page_queue,currPage);
        *(currPage->pte) &= ~PTE_A;
      }
#endif
    }
  }
}
#endif

#ifdef AQ

/*
 *  Queue implementation
 */

//initializes the queue
void initQueue(pageQueue* queue){
  queue->start=0;
  queue->end=0;
  queue->size=0;
  for(int i=0;i<MAX_TOTAL_PAGES;++i){queue->arr[i]=0;}
}

void copyQueue(pageQueue* dst,pageQueue* src,struct page_meta* first_page_dst,struct page_meta* first_page_src){
  dst->start = src->start;
  dst->end = src->end;
  dst->size = src->size;
  for(int i=0;i<MAX_TOTAL_PAGES;++i){
    dst->arr[i] = src->arr[i]==0?0:first_page_dst+(src->arr[i]-first_page_src);
  }
}

//enqueues index to queue
int enqueue(pageQueue* queue,struct page_meta* page){
  if(queue->size==MAX_TOTAL_PAGES){return -1;}
  int next = (queue->end+1)%MAX_TOTAL_PAGES;
  int ans = queue->end;
  queue->arr[ans]=page;
  page->queue_location = ans;
  queue->end = next;
  queue->size++;
  return ans;
}

//dequeues index from queue
struct page_meta* dequeue(pageQueue* queue){
  if(isEmpty(queue)){return 0;}
  struct page_meta* ans = queue->arr[queue->start];
  while(!ans){
    queue->start = (queue->start+1)%MAX_TOTAL_PAGES;
    ans = queue->arr[queue->start];
  }
  queue->arr[queue->start]=0;
  queue->start = (queue->start+1)%MAX_TOTAL_PAGES;
  queue->size--;
  ans->queue_location = -1;
  return ans;
}

//advances a page
void advancePage(pageQueue* queue, struct page_meta* page){
  int ind = page->queue_location;
  if(ind==-1){return;}
  int secInd = (ind+1)%MAX_TOTAL_PAGES;
  while(!queue->arr[secInd]){
    secInd = (secInd+1)%MAX_TOTAL_PAGES;
    if(secInd==queue->end){return;}
  }
  if(secInd==queue->end){return;}
  struct page_meta* tmp = queue->arr[secInd];
  queue->arr[secInd] = queue->arr[ind];
  queue->arr[ind] = tmp;
  page->queue_location = secInd;
  tmp->queue_location = ind;
}

//checks if queue is empty
int isEmpty(pageQueue* queue){
  return queue->size==0;
}

void printQueue(pageQueue* queue, struct page_meta* first_page){
  cprintf("queue size:%d\nfirst:%d\nlast:%d\n",queue->size,queue->start,queue->end);
  if(queue->size==0){return;}
  int curr = queue->start;
  do{
    cprintf("arr[%d] = %p \tpage ind:%d queue ind:%d\n",curr,queue->arr[curr],queue->arr[curr]-first_page,queue->arr[curr]->queue_location);
    curr = (curr+1)%MAX_TOTAL_PAGES;
  }while(curr!=queue->end);
  cprintf("-----------------------\n");
}

#endif
