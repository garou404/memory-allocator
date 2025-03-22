#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include "cmocka.h"
#include "memory_alloc.h"

struct memory_alloc_t m;

/* Initialize the memory allocator */
void memory_init() {
  m.available_blocks = DEFAULT_SIZE;
  m.first_block = 0;
  int i;
  for (i = 0; i < DEFAULT_SIZE-1; i++)
  {
    m.blocks[i] = i+1;
  }
  m.blocks[i] = NULL_BLOCK;
  m.error_no = E_SUCCESS;
}

/* Return the number of consecutive blocks starting from first */
int nb_consecutive_blocks(int first) {
  int index = first;
  int nb_consecutive_blocks = 1;
  while(index+1 == m.blocks[index]){
    nb_consecutive_blocks++;
    index++;
  }
  return nb_consecutive_blocks;
}

/* Reorder memory blocks */
void memory_reorder() {
  for(int i = m.available_blocks; i > 1; i--){
    int index = 0;
    for(int u = 0; u < i-1; u++) {
      if(u==0){
        if(m.first_block > m.blocks[m.first_block]){
          memory_page_t blo = m.blocks[m.first_block];
          memory_page_t bloblo = m.blocks[m.blocks[m.first_block]];
          m.blocks[m.blocks[m.first_block]] = m.first_block;
          m.blocks[m.first_block] = bloblo;
          m.first_block = blo;
        }
        index = m.first_block;
      }else{
        if(m.blocks[index] > m.blocks[m.blocks[index]]){
          memory_page_t blo = m.blocks[index]; 
          memory_page_t bloblo = m.blocks[m.blocks[index]];
          memory_page_t blobloblo = m.blocks[m.blocks[m.blocks[index]]];
          m.blocks[m.blocks[m.blocks[index]]] = blo;
          m.blocks[m.blocks[index]] = blobloblo;
          m.blocks[index] = bloblo;
        }
        index = m.blocks[index];
      }
    }
  }
}

/* Allocate size bytes
 * return NULL_BLOCK in case of an error
 */
int memory_allocate(size_t size) {
  
  int index = m.first_block;
  int block_nb_needed = size / 8;
  if(size % 8 != 0) { block_nb_needed++; }
  if(nb_consecutive_blocks(index) < block_nb_needed){ // case where the needed nb of blocks is not available on first block
    while(nb_consecutive_blocks(m.blocks[index]) < block_nb_needed){
      index = m.blocks[index];
      if(m.blocks[index] == NULL_BLOCK){
        memory_reorder();
        index = m.first_block;
        while(nb_consecutive_blocks(m.blocks[index]) < block_nb_needed){ // we check again after memory reorder
          index = m.blocks[index];
          if(m.blocks[index] == NULL_BLOCK){
            m.error_no = E_SHOULD_PACK;
            return NULL_BLOCK;
          }
        }
        break; // jump out of the while
      }
    }
    int first_block = m.blocks[index];
    m.blocks[index] = m.blocks[first_block + block_nb_needed-1];
    initialize_buffer(first_block, size);
    m.available_blocks-=block_nb_needed;
    m.error_no = E_SUCCESS;
    return first_block;
  }else{ // case where the needed nb of blocks is available on first block
    m.available_blocks-=block_nb_needed;
    m.error_no = E_SUCCESS;
    m.first_block = m.blocks[index + block_nb_needed-1];
    initialize_buffer(index, size);
    return index;
  }
}

/* Free the block of data starting at address */
void memory_free(int address, size_t size) {
  int block_nb = size / 8;
  if(size % 8 != 0) { block_nb++; }
  for (int i = address; i < address+block_nb; i++)
  {
    m.blocks[i] = i+1;
  }
  m.blocks[address+block_nb-1] = m.first_block;
  m.first_block = address;
  m.available_blocks += block_nb;
  m.error_no = E_SUCCESS;
  
}

/* Print information on the available blocks of the memory allocator */
void memory_print() {
  printf("---------------------------------\n");
  printf("\tBlock size: %lu\n", sizeof(m.blocks[0]));
  printf("\tAvailable blocks: %lu\n", m.available_blocks);
  printf("\tFirst free: %d\n", m.first_block);
  printf("\tError_no: "); memory_error_print(m.error_no);
  printf("\tContent:  ");

  int address = m.first_block;
  printf("[%d]", address);
  while(m.blocks[address] != NULL_BLOCK) {
    printf(" -> [%ld]", m.blocks[address]);
    address = m.blocks[address];
  }
  printf(" -> NULL_BLOCK");

  printf("\n");
  printf("---------------------------------\n");
}

int memory_lifelike_malloc(size_t size) {
  // will look for size + 1 blocks
  // will return the addr of the second block
  int index = m.first_block;
  int block_nb_needed = size / 8;
  if(size % 8 != 0) block_nb_needed++;
  block_nb_needed++; // add one block needed to store the nb of blocks
  if(nb_consecutive_blocks(index) < block_nb_needed){ // case where the needed nb of blocks is not available on first block
    while(nb_consecutive_blocks(m.blocks[index]) < block_nb_needed){
      index = m.blocks[index];
      if(m.blocks[index] == NULL_BLOCK){
        memory_reorder();
        index = m.first_block;
        while(nb_consecutive_blocks(m.blocks[index]) < block_nb_needed){ // we check again after memory reorder
          index = m.blocks[index];
          if(m.blocks[index] == NULL_BLOCK){
            m.error_no = E_SHOULD_PACK;
            return NULL_BLOCK;
          }
        }
        break; // jump out of the while
      }
    }
    int first_block = m.blocks[index];
    m.blocks[index] = m.blocks[first_block + block_nb_needed-1];
    m.blocks[first_block] = block_nb_needed*8; // Store the size in bytes needed
    initialize_buffer(first_block+1, size);
    m.available_blocks-=block_nb_needed;
    m.error_no = E_SUCCESS;
    return first_block+1;  
  }else{ // case where the needed nb of blocks is available on first block
    m.available_blocks-=block_nb_needed;
    m.error_no = E_SUCCESS;
    m.first_block = m.blocks[index + block_nb_needed-1];
    m.blocks[index] = block_nb_needed-1; // Store the size in bytes needed
    initialize_buffer(index+1, size);
    m.error_no = E_SUCCESS;
    return index+1;
  }
  m.error_no = E_SHOULD_PACK;
  return NULL_BLOCK;
}

void memory_lifelike_free(int addr) {
  int block_nb = m.blocks[addr-1]/8;
  for (int i = addr-1; i < (addr-1)+block_nb-1; i++) {
    m.blocks[i] = i+1;
  }
  m.blocks[(addr-1)+block_nb-1] = m.first_block;
  m.first_block = addr-1;
  m.available_blocks += block_nb;
  m.error_no = E_SUCCESS;
}

int memory_lifelike_realloc(int addr, size_t size){
  if(size == 0) { // behave like free
    memory_lifelike_free(addr);
    m.error_no = E_SUCCESS;
    return addr;
  }
  if(addr == NULL_BLOCK) { // behave like malloc
    m.error_no = E_SUCCESS;
    return memory_lifelike_malloc(size);
  }

  int block_nb_needed = size/8 + 1; // add one block to store the size
  if(size % 8 != 0) block_nb_needed++;
  if(block_nb_needed*8 == m.blocks[addr-1]) {
    m.error_no = E_SUCCESS;
    return addr; // same size do nothing 
  }else if(block_nb_needed*8 < m.blocks[addr-1]){ // new size < cur size
    int nb_blocks_del = m.blocks[addr-1]/8 - block_nb_needed;
    m.available_blocks+=m.blocks[addr-1]/8-block_nb_needed;
    m.blocks[addr-1] = block_nb_needed*8;
    for (int i=0; i < nb_blocks_del-1; i++) {
      m.blocks[addr+block_nb_needed-1+i] = addr+block_nb_needed+i;
    }
    m.blocks[addr+block_nb_needed-2+nb_blocks_del] = m.first_block;
    m.first_block = addr+block_nb_needed-1;
    m.error_no = E_SUCCESS;
    return addr;
  }else { // new size > cur size
    if(nb_consecutive_blocks(addr+(m.blocks[addr-1]/8)-1) < block_nb_needed-m.blocks[addr-1]/8) { // not enough space next to cur addr
      memory_lifelike_free(addr);
      return memory_lifelike_malloc((block_nb_needed-1)*8);
    }else{ // enough space next to cur
      // printf("// enough space next to cur\n");
      int index = m.first_block;
      // printf("addr+m.blocks[addr-1]/8-1 %ld\n", (addr+m.blocks[addr-1]/8-1));
      while(m.blocks[index] != addr+m.blocks[addr-1]/8-1) index = m.blocks[index];
      // printf("index: %d\n", index);
      // printf("m.blocks[index] new value : %d\n", addr+block_nb_needed-1);
      m.available_blocks-=block_nb_needed-m.blocks[addr-1]/8;
      m.blocks[addr-1] = block_nb_needed*8;
      m.blocks[index] = addr+block_nb_needed-1;
      m.error_no = E_SUCCESS;
      return addr;
    }
  }
}

/* print the message corresponding to error_number */
void memory_error_print(enum memory_errno error_number) {
  switch(error_number) {
  case E_SUCCESS:
    printf("Success\n");
    break;
  case E_NOMEM:
    printf("Not enough memory\n");
    break;
  case  E_SHOULD_PACK:
    printf("Not enough contiguous blocks\n");
    break;
  default:
    printf("Unknown\n");
    break;
  }
}

/* Initialize an allocated buffer with zeros */
void initialize_buffer(int start_index, size_t size) {
  char* ptr = (char*)&m.blocks[start_index];
  for(int i=0; i<size; i++) {
    ptr[i]=0;
  }
}

/*************************************************/
/*             Test functions                    */
/*************************************************/

// We define a constant to be stored in a block which is supposed to be allocated:
// The value of this constant is such as it is different form NULL_BLOCK *and*
// it guarantees a segmentation vioaltion in case the code does something like
// m.blocks[A_B]
#define A_B INT32_MIN

/* Initialize m with all allocated blocks. So there is no available block */
void init_m_with_all_allocated_blocks() {
  struct memory_alloc_t m_init = {
    // 0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
    {A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B},
    0,
    NULL_BLOCK,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

/* Test memory_init() */
void test_exo1_memory_init(){
  init_m_with_all_allocated_blocks();

  memory_init();

  // Check that m contains [0]->[1]->[2]->[3]->[4]->[5]->[6]->[7]->[8]->[9]->[10]->[11]->[12]->[13]->[14]->[15]->NULL_BLOCK
  assert_int_equal(0, m.first_block);

  assert_int_equal(1, m.blocks[0]);
  assert_int_equal(2, m.blocks[1]);
  assert_int_equal(3, m.blocks[2]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(6, m.blocks[5]);
  assert_int_equal(7, m.blocks[6]);
  assert_int_equal(8, m.blocks[7]);
  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(10, m.blocks[9]);
  assert_int_equal(11, m.blocks[10]);
  assert_int_equal(12, m.blocks[11]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(15, m.blocks[14]);
  assert_int_equal(NULL_BLOCK, m.blocks[15]);
  
  assert_int_equal(DEFAULT_SIZE, m.available_blocks);

  // We do not care about value of m.error_no
}

/* Initialize m with some allocated blocks. The 10 available blocks are: [8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK */
void init_m_with_some_allocated_blocks() {
  struct memory_alloc_t m_init = {
    // 0           1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
    {A_B, NULL_BLOCK, A_B,   4,   5,  12, A_B, A_B,   9,   3, A_B,   1,  13,  14,  11, A_B},
    10,
    8,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

/* Test nb_consecutive_block() at the beginning of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_beginning_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(2, nb_consecutive_blocks(8));
}

/* Test nb_consecutive_block() at the middle of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_middle_linked_list(){
  init_m_with_some_allocated_blocks();
  // memory_print();
  assert_int_equal(3, nb_consecutive_blocks(3));
}

/* Test nb_consecutive_block() at the end of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_end_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(1, nb_consecutive_blocks(1));
}

/* Test memory_allocate() when the blocks allocated are at the beginning of the linked list */
void test_exo1_memory_allocate_beginning_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(8, memory_allocate(16));
  
  // We check that m contains [3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK 
  assert_int_equal(3, m.first_block);

  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(12, m.blocks[5]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(11, m.blocks[14]);
  assert_int_equal(1, m.blocks[11]);
  assert_int_equal(NULL_BLOCK, m.blocks[1]);
  
  assert_int_equal(8, m.available_blocks);

  assert_int_equal(E_SUCCESS, m.error_no);
}

/* Test memory_allocate() when the blocks allocated are at in the middle of the linked list */
void test_exo1_memory_allocate_middle_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(3, memory_allocate(17));
  
  // We check that m contains [8]->[9]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK 
  assert_int_equal(8, m.first_block);

  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(12, m.blocks[9]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(11, m.blocks[14]);
  assert_int_equal(1, m.blocks[11]);
  assert_int_equal(NULL_BLOCK, m.blocks[1]);
  
  assert_int_equal(7, m.available_blocks);

  assert_int_equal(E_SUCCESS, m.error_no);
}

/* Test memory_allocate() when we ask for too many blocks ==> We get NULL_BLOCK return code and m.error_no is M_NOMEM */
void test_exo1_memory_allocate_too_many_blocks(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(NULL_BLOCK, memory_allocate(256));
  
  // We check that m does not change and still contains: [8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK
  assert_int_equal(8, m.first_block);

  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(3, m.blocks[9]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(12, m.blocks[5]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(11, m.blocks[14]);
  assert_int_equal(1, m.blocks[11]);
  assert_int_equal(NULL_BLOCK, m.blocks[1]);
  
  assert_int_equal(10, m.available_blocks);

  assert_int_equal(E_NOMEM, m.error_no);
}

/* Test memory_free() */
void test_exo1_memory_free(){
  init_m_with_some_allocated_blocks();

  memory_free(6, 9);
  
  // We check that m contains: [6]->[7]->[8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK
  assert_int_equal(6, m.first_block);

  assert_int_equal(7, m.blocks[6]);
  assert_int_equal(8, m.blocks[7]);
  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(3, m.blocks[9]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(12, m.blocks[5]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(11, m.blocks[14]);
  assert_int_equal(1, m.blocks[11]);
  assert_int_equal(NULL_BLOCK, m.blocks[1]);
  
  assert_int_equal(12, m.available_blocks);

  // We do not care about value of m.error_no
}

/* Test memory_reorder() */
void test_exo2_memory_reorder(){
  init_m_with_some_allocated_blocks();
  memory_reorder();

  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->[11]->[12]->[13]->[14]->NULL_BLOCK
  assert_int_equal(1, m.first_block);

  assert_int_equal(3, m.blocks[1]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(8, m.blocks[5]);
  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(11, m.blocks[9]);
  assert_int_equal(12, m.blocks[11]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(NULL_BLOCK, m.blocks[14]);  
  assert_int_equal(10, m.available_blocks);

  // We do not care about value of m.error_no
}

/* Test memory_reorder() leading to successful memory_allocate() because we find enough consecutive bytes */
void test_exo2_memory_reorder_leading_to_successful_memory_allocate(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(11, memory_allocate(32));
  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->NULL_BLOCK
  assert_int_equal(1, m.first_block);

  assert_int_equal(3, m.blocks[1]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(8, m.blocks[5]);
  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(NULL_BLOCK, m.blocks[9]);
  
  assert_int_equal(6, m.available_blocks);

  assert_int_equal(E_SUCCESS, m.error_no);
}

/* Test memory_reorder() leading to failed memory_allocate() because not enough consecutive bytes */
void test_exo2_memory_reorder_leading_to_failed_memory_allocate(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(NULL_BLOCK, memory_allocate(56));

  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->[11]->[12]->[13]->[14]->NULL_BLOCK
  assert_int_equal(1, m.first_block);

  assert_int_equal(3, m.blocks[1]);
  assert_int_equal(4, m.blocks[3]);
  assert_int_equal(5, m.blocks[4]);
  assert_int_equal(8, m.blocks[5]);
  assert_int_equal(9, m.blocks[8]);
  assert_int_equal(11, m.blocks[9]);
  assert_int_equal(12, m.blocks[11]);
  assert_int_equal(13, m.blocks[12]);
  assert_int_equal(14, m.blocks[13]);
  assert_int_equal(NULL_BLOCK, m.blocks[14]);  
  assert_int_equal(10, m.available_blocks);

  assert_int_equal(10, m.available_blocks);

  assert_int_equal(E_SHOULD_PACK, m.error_no);
}

/* Initialize m with some allocated blocks. The 10 available blocks are: [0]->[1]->[4]->[5]->[9]->[10]->[15]->NULL_BLOCK */
void init_m_with_some_allocated_blocks_lifelike() {
  struct memory_alloc_t m_init = {
    // 0 1   2     3      4    5    6    7    8     9   10    11   12    13    14    15
    {1,  4,  16,   A_B,   5,   6,   7,   8,   9,   10,  15,   32,  A_B,  A_B,  A_B,  NULL_BLOCK},
    10,
    0,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

/* Initialize m with some allocated blocks. The 10 available blocks are: [0]->[1]->[4]->[5]->[9]->[10]->[15]->NULL_BLOCK */
void init_m_with_some_allocated_blocks_lifelike_free_old_area() {
  struct memory_alloc_t m_init = {
    // 0 1   2    3       4    5      6    7    8     9    10   11   12    13    14    15
    {1,  6,  16,   A_B,   16,   A_B,   7,   8,   9,   10,  15,  32,  A_B,  A_B,  A_B,  NULL_BLOCK},
    8,
    0,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}


/* Initialize m with some allocated blocks. The 10 available blocks are: [0]->[1]->[4]->[5]->[9]->[10]->[15]->NULL_BLOCK */
void init_m_with_some_allocated_blocks_lifelike_not_enough_space() {
  struct memory_alloc_t m_init = {
    // 0 1    2      3      4     5    6    7     8     9     10    11    12   13           14  15
    {5,  32,  A_B,   A_B,   A_B,  12,  48,  A_B,  A_B,  A_B,  A_B,  A_B,  13,  NULL_BLOCK,  8,  A_B},
    4,
    0,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

void test_exo3_memory_alloc_lifelike(){
  init_m_with_some_allocated_blocks_lifelike();
  size_t block_needed = 4;
  int initial_block_available = m.available_blocks;
  int addr = memory_lifelike_malloc(block_needed*8);
  // check that we remove the n + 1 blocks required from the available counter
  assert_int_equal(initial_block_available-block_needed-1, m.available_blocks);
  // printf("m.blocks[addr-1] %ld, block_needed*8+8 %ld, m.blocks[addr] %ld\n", m.blocks[addr-1], block_needed*8+8, m.blocks[addr]);
  // check that the nb of bytes required is stored in addr-1
  assert_int_equal(m.blocks[addr-1], block_needed*8+8);
  assert_int_equal(m.blocks[1], 9);
  assert_int_equal(E_SUCCESS, m.error_no);
}

void test_exo3_memory_free_lifelike(){
  init_m_with_some_allocated_blocks_lifelike();
  size_t block_needed = 4;
  int addr = memory_lifelike_malloc(block_needed*8);
  int block_available_after_malloc = m.available_blocks;
  memory_lifelike_free(addr);
  // check that we have the good nb of available blocks after free
  assert_int_equal(block_available_after_malloc+block_needed+1, m.available_blocks);
  assert_int_equal(m.first_block, 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 6);
  assert_int_equal(m.blocks[6], 7);
  assert_int_equal(m.blocks[7], 8);
  assert_int_equal(m.blocks[8], 0);
  assert_int_equal(E_SUCCESS, m.error_no);
}


void test_exo3_memory_realloc_lifelike_new_size_sup(){
  init_m_with_some_allocated_blocks_lifelike();
  int addr = 3;
  int prev_available_blocks = m.available_blocks;
  memory_lifelike_realloc(addr, 24);
  assert_int_equal(m.blocks[addr-1], 32);
  assert_int_equal(m.blocks[1], 6);
  assert_int_equal(m.available_blocks+2, prev_available_blocks);
  assert_int_equal(E_SUCCESS, m.error_no);

}

void test_exo3_memory_realloc_lifelike_new_size_equ(){
  init_m_with_some_allocated_blocks_lifelike();
  int addr = 3;
  int prev_available_blocks = m.available_blocks;
  memory_lifelike_realloc(addr, 8);
  assert_int_equal(m.blocks[addr-1], 16);
  assert_int_equal(m.blocks[1], 4);
  assert_int_equal(m.available_blocks, prev_available_blocks);
  assert_int_equal(E_SUCCESS, m.error_no);

}

void test_exo3_memory_realloc_lifelike_new_size_inf(){
  init_m_with_some_allocated_blocks_lifelike();
  int addr = 12;
  int prev_available_blocks = m.available_blocks;
  memory_lifelike_realloc(addr, 8);
  assert_int_equal(m.blocks[addr-1], 16);
  assert_int_equal(m.first_block, 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 0);
  assert_int_equal(m.available_blocks-2, prev_available_blocks);
  assert_int_equal(E_SUCCESS, m.error_no);
}

void test_exo3_memory_realloc_lifelike_size_null(){
  init_m_with_some_allocated_blocks_lifelike();
  int addr = 3;
  int prev_available_blocks = m.available_blocks;
  memory_lifelike_realloc(addr, 0); // behave like free
  assert_int_equal(m.first_block, 2);
  assert_int_equal(m.blocks[2], 3);
  assert_int_equal(m.blocks[3], 0);
  assert_int_equal(m.available_blocks, prev_available_blocks+2);
}

void test_exo3_memory_realloc_lifelike_addr_null_block(){
  init_m_with_some_allocated_blocks_lifelike();
  int prev_available_blocks = m.available_blocks;
  memory_lifelike_realloc(NULL_BLOCK, 32); //behave like malloc
  assert_int_equal(m.blocks[1], 9);
  assert_int_equal(m.blocks[4], 40);
  assert_int_equal(m.available_blocks, prev_available_blocks-5);
}

void test_exo3_memory_realloc_lifelike_old_area_free(){
  init_m_with_some_allocated_blocks_lifelike_free_old_area();
  int addr = 3;
  int prev_available_blocks = m.available_blocks;
  int new_addr = memory_lifelike_realloc(addr, 3*8);
  assert_int_equal(m.first_block, 2);
  assert_int_equal(m.blocks[2], 3);
  assert_int_equal(m.blocks[3], 0);
  assert_int_equal(new_addr, 7); // check address returned is good
  assert_int_equal(m.blocks[6], 4*8); 
  assert_int_equal(m.blocks[1], 10); 
  assert_int_equal(m.available_blocks, prev_available_blocks-2);
}

void test_exo3_memory_realloc_lifelike_not_enough_memory(){
  init_m_with_some_allocated_blocks_lifelike_not_enough_space();
  int addr = 2;
  int new_addr = memory_lifelike_realloc(addr, 48);
  assert_int_equal(new_addr, NULL_BLOCK);
  assert_int_equal(E_SHOULD_PACK, m.error_no);
}


int main(int argc, char**argv) {
  const struct CMUnitTest tests[] = {
    /* a few tests for exercise 1.
     *
     * If you implemented correctly the functions, all these tests should be successfull
     * Of course this test suite may not cover all the tricky cases, and you are free to add
     * your own tests.
     */
    cmocka_unit_test(test_exo1_memory_init),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_beginning_linked_list),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_middle_linked_list),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_end_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_beginning_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_middle_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_too_many_blocks),
    cmocka_unit_test(test_exo1_memory_free),

    /* Run a few tests for exercise 2.
     *
     * If you implemented correctly the functions, all these tests should be successfull
     * Of course this test suite may not cover all the tricky cases, and you are free to add
     * your own tests.
     */

    cmocka_unit_test(test_exo2_memory_reorder),
    cmocka_unit_test(test_exo2_memory_reorder_leading_to_successful_memory_allocate),
    cmocka_unit_test(test_exo2_memory_reorder_leading_to_failed_memory_allocate),

    /* a few tests for exercise 3.
     */
    // test that when malloc check val at addr-1 is the size that was asked when malloc
    // mem allocate check nb of available blocks 
    // free check nb of available blocks again
    // malloc 2 blocks while only 2 clocks consecutive are available => error
    cmocka_unit_test(test_exo3_memory_alloc_lifelike),
    cmocka_unit_test(test_exo3_memory_free_lifelike),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_new_size_sup),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_new_size_equ),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_new_size_inf),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_size_null),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_addr_null_block),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_old_area_free),
    cmocka_unit_test(test_exo3_memory_realloc_lifelike_not_enough_memory)

  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
