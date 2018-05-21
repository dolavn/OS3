struct run {
  struct run *next;
};

void add_total_pages_num(int);
void count_pages(char*);

void dec_free_pages();
void inc_free_pages();

int get_total_pages_num();
int get_free_pages_num();