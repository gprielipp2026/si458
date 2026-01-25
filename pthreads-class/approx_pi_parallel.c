#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

// pthread library
#include <pthread.h>

// information to pass the thread
typedef struct {
  int id, num;
  double* pi; // pointer to an array that will contain each pi value
} thread_info;

int in_circle(double x, double y) {
    if (y*y <= 1 - x*x) return 1;

    return 0;
}


double approx_pi(int n) {
    int count = 0;

    for (int i = 0; i < n; i++) {
        double x = (double) rand() / (double) RAND_MAX;
        double y = (double) rand() / (double) RAND_MAX;

        count += in_circle(x,y);
    }

    return 4.0* (double) count / (double) n;
}

// parallel function
void* parallelized(void* arg) {
  thread_info* info = (thread_info*)arg;
  info->pi[info->id] = approx_pi(info->num);

  return NULL;
}

int main(int argc, char* argv[]) {
    /* 
    input:
        num_pts (int)
        num_threads (int) - default: 1
    */
    int num_threads = 1;

    if (argc != 2){
        printf("useage: %s <num_pts> <num_threads: default=1>\n", argv[0]);
    }
    if (argc == 3) num_threads = atoi(argv[2]);
    
    int n = atoi(argv[1]);

    srand(time(NULL));

    // start threads
    pthread_t threads [num_threads];
    thread_info info[num_threads];
    double* pis = malloc(sizeof(double)*num_threads);

    // create and run threads
    int leftover = n % num_threads;
    for(int i = 0; i < num_threads; i++) {
      info[i].id = i;
      info[i].num = n / num_threads;
      info[i].pi = pis;

      if(pthread_create(&threads[i], NULL, parallelized, (void*)&info[i]) != 0) {
        fprintf(stderr, "Error: thread %d not created \n", i);
        exit(1);
      }
    }
   
    // do work while threads are working
    double sum = approx_pi(leftover);

    // join all of the threads
    for(int i = 0; i < num_threads; i++) {
      pthread_join(threads[i], NULL);
    }

    // average all of the pi's

    for(int i = 0; i < num_threads; i++) {
      sum += pis[i];
    }

    free(pis);

    double pi = sum / (num_threads + (leftover > 0 ? 1 : 0));

    printf("pi: %f\n", pi);

    return 0;
}

