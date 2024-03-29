// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

#ifndef NONE
struct page_meta{
  uint* pte;
  char* va;
  int offset;
  char on_phys;
  char taken;
#if defined(NFUA) || defined(LAPA)
  uint counter;
#endif
#ifdef AQ
  uint queue_location;
#endif
#ifdef SCFIFO
  int nextp; // index of next in pages
  int prevp;
#endif
};
#endif

#ifdef AQ
typedef struct pageQueue{
  struct page_meta* arr[MAX_TOTAL_PAGES];
  int start;
  int end;
  int size;
}pageQueue;

void initQueue(pageQueue*);
void copyQueue(pageQueue*,pageQueue*,struct page_meta*,struct page_meta*);
int enqueue(pageQueue*,struct page_meta*);
struct page_meta* dequeue(pageQueue*);
int isEmpty(pageQueue*);
void advancePage(pageQueue*,struct page_meta*);
void printQueue(pageQueue*,struct page_meta*);
#endif

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
#ifdef AQ
  struct pageQueue page_queue;
#endif
  //Swap file. must initiate with create swap file
  struct file *swapFile;      //page file
  uint num_of_pages;
  uint phys_pages;
  uint pgflt_count;
  uint pgout_count;
  char ignorePaging;
#ifndef NONE
  int file_size;              //file size
  struct page_meta pages[MAX_TOTAL_PAGES];
  int offsets[MAX_SWAP_FILE_SZ];
#ifdef SCFIFO
  int headp;  // index of head in pages
#endif
#endif
};

#ifndef NONE
void free_page(struct proc*);
int find_free_slot();
int find_page_ind(struct proc*,uint*);
int swap_to_file(uint*);
int swap_from_file(uint*);
int get_offset(struct proc*);
void free_offset(struct proc*,int);
void copy_swap_file(struct proc*,struct proc*);
int get_page_to_swap();
#endif
int init_page_meta(struct proc*);
int add_page(struct proc*,uint*,char*);
int remove_page(uint*);
void printbits(uint*);
void copy_page_arr(struct proc*,struct proc*);
void handle_pgflt();

void print_proc_data(struct proc*);

#if defined(NFUA) || defined(LAPA) || defined(AQ)
void updatePagesCounter();
#endif

#ifdef LAPA
uint num_of_ones(uint);
#endif



// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
