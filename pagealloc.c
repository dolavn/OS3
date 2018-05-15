#include "pagealloc.h"

int total_pages = 0;
int free_pages = 0;

void add_total_pages_num(int num){
  total_pages = total_pages + num;
  free_pages = total_pages;
}

void dec_free_pages(){
  free_pages--;
}

void inc_free_pages(){
  free_pages++;
}

int get_total_pages_num(){
  return total_pages;
}

int get_free_pages_num(){
  return free_pages;
}