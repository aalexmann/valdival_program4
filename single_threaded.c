#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h> 

/*

A program with a pipeline of 4 threads that interact with each other as producers 
   and consumers:

1. Input thread is the first thread in the pipeline. It holds every character from the 
    user input, up until the new line character and then puts the line into buffer_1, 
    which it shares with the next thread in the pipeline.
    This first thread will stop processing once it reads and whole input line that
    says only "STOP", immediately followed by a line separator(newline).

2. Line Separator Thread gets and reads lines from buffer_1, replacing every line
    separator ("\n") with a space. It will then forward the line to buffer_2.

3. Plus Sign Thread gets and reads lines from buffer_2, replacing every pair of plus 
    signs ("++") with a "^" symbol. It will then forward the line to buffer_3.

4. Output Thread will take lines from buffer_3, and formulate them to lines of exactly
    80 characters before writing them to standard output.

*/


// Size of the buffers
#define MAX_CHAR 1000
// Number of lines that will be produced. This number is less than the size of the buffer. Hence, we can model the buffer as being unbounded.
#define MAX_LINES 50
// Special flag used to indicate that a STOP process has occurred.
int end_processing = 0;
// Special flag used to indicate that a plus was found in the previous character slot.
int prev_plus = 0;

// Buffer 1, shared resource between input thread and line separator thread
int buffer_1[MAX_CHAR];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int produced_idx_1 = 0;
// Index where the line separator thread will pick up the next item
int consumed_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t not_empty_1 = PTHREAD_COND_INITIALIZER;

// Buffer 2, shared resource between line separator thread and plus sign thread
int buffer_2[MAX_CHAR];
// Number of items in the buffer
int count_2 = 0;
// Index where the plus sign thread will put the next item
int produced_idx_2 = 0;
// Index where the plus sign thread will pick up the next item
int consumed_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t not_empty_2 = PTHREAD_COND_INITIALIZER;

// Buffer 3, shared resource between plus sign thread and output thread
int buffer_3[MAX_CHAR];
// Number of items in the buffer
int count_3 = 0;
// Index where the plus sign thread will put the next item
int produced_idx_3 = 0;
// Index where the plus sign thread will pick up the next item
int consumed_idx_3 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t not_empty_3 = PTHREAD_COND_INITIALIZER;

/*
Get input from the user.
This function doesn't perform any error checking.
*/
char* get_user_input(){
  char* value;
  scanf("%s", &value);
  return value;
}

/*
 Put an item in buffer_1
*/
void put_buff_1(char item){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_1);
  // Put the item in the buffer
  buffer_1[produced_idx_1] = item;
  // Increment the index where the next item will be put.
  produced_idx_1 = produced_idx_1 + 1;
  count_1++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&not_empty_1);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_1);
}

/*
 Function that the input thread will run.
 Get input from the user.
 Put the item in the buffer shared with the line separator thread.
*/
void *get_input(void *args)
{
    // Get the user input
    char character[] = get_user_input();
    for (int i = 0; i < MAX_CHAR; i++)
    {
      put_buff_1(character[i]);
      if (stop_processing(character, i)){
          end_processing = 1;
          break;
      }  
    }
    return NULL;
}

/*
 Function that reads a section of
 the input from the user and returns
 true if "STOP\n" is next, false otherwise.
*/
int stop_processing(char letter[], int idx)
{
    if (letter[idx + 1] == "S"){
        if (letter[idx + 2] == "T"){
            if (letter[idx + 3] == "O"){
                if (letter[idx + 4] == "P"){
                    if (letter[idx + 5] == "\n"){
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/*
Get the next item from buffer_1
*/
char * get_buff_1(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_1);
  while (count_1 == 0) {
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&not_empty_1, &mutex_1);
  }
  char *item = buffer_1[consumed_idx_1];
  // Increment the index from which the item will be picked up
  consumed_idx_1 = consumed_idx_1 + 1;
  count_1--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_1);
  // Return the item
  return item;
}

/*
 Put an item in buffer_2
*/
void put_buff_2(char item){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_2);
  // Put the item in the buffer
  buffer_2[produced_idx_2] = item;
  // Increment the index where the next item will be put.
  produced_idx_2 = produced_idx_2 + 1;
  count_2++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&not_empty_2);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
}

/*
 Function that the line separator thread will run. 
 Consume an item from buffer_1 shared with the input thread.
 Replace any newline character with a space.
 Produce an item in buffer_2 shared with the plus sign thread.

*/
void *separate_line(void *args)
{
    char item;
    char space = " ";
    for (int i = 0; i < MAX_CHAR; i++)
    {
      item = get_buff_1();
      if (item == "\n"){
          put_buff_2(space);
      }
      else {
          put_buff_2(item);
      }
    }
    return NULL;
}

/*
Get the next item from buffer_2
*/
char * get_buff_2(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_2);
  while (count_2 == 0) {
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&not_empty_2, &mutex_2);
  }
  char *item = buffer_2[consumed_idx_2];
  // Increment the index from which the item will be picked up
  consumed_idx_2 = consumed_idx_2 + 1;
  count_2--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
  // Return the item
  return item;
}

/*
 Put an item in buffer_3
*/
void put_buff_3(char item){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_3);
  // Put the item in the buffer
  buffer_3[produced_idx_3] = item;
  // Increment the index where the next item will be put.
  produced_idx_3 = produced_idx_3 + 1;
  count_3++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&not_empty_3);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_3);
}

/*
 Function that the plus sign thread will run. 
 Consume an item from buffer_2 shared with the line separator thread.
 Replace any consecutive plusses (++) with a single "^".
 Produce an item in buffer_3 shared with the output thread.
*/
void *plusChange(void *args)
{
    char item;
    char exp = "^";
    for (int i = 0; i < MAX_CHAR; i++)
    {
      item = get_buff_2();
      if (item == "+"){
          if (check_next()){
              put_buff_3(exp);
              consumed_idx_2 = consumed_idx_2 + 1;
          }
      }
      else {
          put_buff_3(item);
      }
    }
    return NULL;
}

/*
 Helper fucntion for plusChange()
 that checks the next character present in buffer_2 
 and returns true if it is a "+", false if not.
*/
int check_next()
{
    // Lock the mutex before checking if the next is a plus.
    pthread_mutex_lock(&mutex_2);
    if (buffer_2[consumed_idx_2] == "+"){
        // Unlock the mutex
        pthread_mutex_unlock(&mutex_2);
        return 1;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
    return 0;
}

/*
Get the next item from buffer_3
*/
char * get_buff_3(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_3);
  while (count_3 == 0) {
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&not_empty_3, &mutex_3);
  }
  char *item = buffer_3[consumed_idx_3];
  // Increment the index from which the item will be picked up
  consumed_idx_3 = consumed_idx_3 + 1;
  count_3--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_3);
  // Return the item
  return item;
}

/*
 Function that the output thread will run. 
 Consume an item from the buffer shared with the square root thread.
 Print the item.
*/
void *write_output(void *args)
{
    char line_eighty[80];
    for (int i = 0; i < MAX_CHAR; i++)
    {
        if (i < 80){
            line_eighty[i] = get_buff_3();
            if ( i == 79){
                printf("%s\n", line_eighty);
                int k = 0;
                while (k < 80){
                    line_eighty[k] = NULL;
                    k++;
                }
            }
        }
        if (i >= 80){
            line_eighty[i % 80] = get_buff_3;
            if ( i % 80 == 0){
                printf("%s\n", line_eighty);
                int f = 0;
                while (f < 80){
                    line_eighty[f] = NULL;
                    f++;
                }
            }
        }
    }
    return NULL;
}

int main()
{
    srand(time(0));
    pthread_t input_t, line_separate_t, plusses_t, output_t;
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_separate_t, NULL, separate_line, NULL);
    pthread_create(&plusses_t, NULL, plusChange, NULL);
    pthread_create(&output_t, NULL, write_output, NULL);
    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_separate_t, NULL);
    pthread_join(plusses_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}