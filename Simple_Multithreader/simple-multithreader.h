#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h> // For threading
#include <sys/time.h>     // For timing execution 


//pthread_create allows to pass one argument (void* pointer)
//we need to bundel the arguments up in a single structure
typedef struct{
  int low;
  int high;
  std::function<void(int)> lambda;
} thread_arguments_one_dimension;
//for single loop operation (vector addition)


typedef struct{
  int low1;
  int high1;
  int low2;
  int high2;
  std::function<void(int, int)> lambda;
} thread_argumnets_two_dimension;
//matrix multiplication


void* thread_function_one_dimension(void* p){
  thread_arguments_one_dimension* t=(thread_arguments_one_dimension*)p;
  for(int i=t->low;i<t->high;i++){
    t->lambda(i);
  }
  return NULL;
}
void* thread_function_two_dimention(void* p) {
  thread_argumnets_two_dimension* t = (thread_argumnets_two_dimension*)p;//unpack
  for (int i = t->low1; i < t->high1; i++) {
    for (int j = t->low2; j < t->high2; j++) {
      t->lambda(i, j); // Call the lambda with both indices
    }
  }
  return NULL;
}

void parallel_for(int low,int high,std::function<void(int)> &&lambda,int Nthreads){
  struct timeval st,ed;
  gettimeofday(&st,NULL);
  //start the timer
  pthread_t *threads=new pthread_t[Nthreads];
  //this keeps the track of the threads
  thread_arguments_one_dimension *args = new thread_arguments_one_dimension[Nthreads];
  //this keeps the argument of for each thread
  int range=high-low;
  int chunk=range/Nthreads;//if perfectly divisible
  int remainder=range%Nthreads;
  int starter=low;
  for(int i=0;i<Nthreads;i++){
    args[i].low=starter;
    //by this starting threads takes the leftover items
    int r=0;
    //as remainder is always less then Nthreads if it is non zero starting threads will tackel it
    if(i<remainder){
      r=1;
    }
    else{
      r=0;
    }
    int size=chunk+r;
    args[i].high=starter+size;
    args[i].lambda=lambda;//pass the user code
    pthread_create(&threads[i], NULL, thread_function_one_dimension, &args[i]);
    starter=args[i].high;//so that next thread can evaluate where the previous left
  }
  //wait for everyone to finish
  for (int i = 0; i < Nthreads; i++) {
    pthread_join(threads[i], NULL);
  }//main thread pauses while the threads works
  gettimeofday(&ed, NULL);
  double elapsed_ms = ((ed.tv_sec - st.tv_sec) * 1000.0) + ((ed.tv_usec - st.tv_usec) / 1000.0);

  std::cout << "Parallel execution time for " << Nthreads << " threads: " << elapsed_ms << " ms" << std::endl;
  delete[] threads;
  delete[] args;
}

//for matrix
void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int NThreads) {
  struct timeval st, ed;
  gettimeofday(&st, NULL);
  pthread_t *threads = new pthread_t[NThreads];
  thread_argumnets_two_dimension *args = new thread_argumnets_two_dimension[NThreads];
  //spliting the rows
  int range = high1 - low1;
  int chunk = range / NThreads;
  int remainder = range % NThreads;
  int starter = low1;
  for (int i = 0; i < NThreads; i++) {
    args[i].low1 = starter;
    int r = (i < remainder) ? 1 : 0;
    int size = chunk + r;
    args[i].high1 = starter + size;
    //for inner loop for columns
    args[i].low2 = low2;
    args[i].high2 = high2;
    args[i].lambda = lambda;
    pthread_create(&threads[i], NULL, thread_function_two_dimention, &args[i]);
    starter = args[i].high1;
  }
  for (int i = 0; i < NThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  gettimeofday(&ed, NULL);
  long seconds = (ed.tv_sec - st.tv_sec);
  double elapsed_ms = ((ed.tv_sec - st.tv_sec) * 1000.0) + ((ed.tv_usec - st.tv_usec) / 1000.0);

  std::cout << "Parallel execution time for " << NThreads << " threads: " << elapsed_ms << " ms" << std::endl;
  delete[] threads;
  delete[] args;
}

int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main