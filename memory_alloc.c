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
  /* TODO (exercise 3) */
  return NULL_BLOCK;
}

void memory_lifelike_free(int addr) {
  /* TODO (exercise 3) */
}

int memory_lifelike_realloc(int addr, size_t size){
  /* TODO (exercise 3) */
  return NULL_BLOCK;
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
    cmocka_unit_test(test_exo2_memory_reorder_leading_to_failed_memory_allocate)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
