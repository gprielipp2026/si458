#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <omp.h>

int sum(int *a, int n) {
  // get thread info
  int my_rank      = omp_get_thread_num();
  int thread_count = omp_get_num_threads();

  // start??? and num_per_thread???
  int num_per_thread = n/thread_count;
  int start          = my_rank * num_per_thread;
  int extras         = 0;
  if (my_rank == thread_count - 1){
    extras = n % thread_count;
  }

  int total = 0;
  for (int i = start; i< start + num_per_thread + extras; i++) {
    total += a[i];
  }

  return total;
}


void print_arr(int *a, int num) {
  for (int i = 0; i < num; i++) printf("%d ", a[i]);
  printf("\n");
}

int* create_arr(int num) {
  int* a = malloc( sizeof(int)* num);
  for (int i = 0; i < num; i++) {
    a[i] = rand() % 10;
  }
  return a;
}

int main(int argc, char *argv[]) {
  /*
input:
num (int): lenght of array
*/
  if (argc != 2) { 
    printf("useage: %s <length of array>\n", argv[0]);
    exit(1);
  }

  srand(1);
  int num = atoi(argv[1]);

  int *a = create_arr(num);
  //    print_arr(a, num);

  int global_tot = 0;

  // sum up all the values
  double start = omp_get_wtime();
/*#   pragma omp parallel 
  {
    int local_tot = 0;
    local_tot += sum(a, num);
#   pragma omp critical
    global_tot += local_tot;
  }
*/

# pragma omp parallel \
    reduction(+: global_tot)
    global_tot += sum(a, num);

  double end = omp_get_wtime();

  printf("total: %d\n", global_tot);
  printf("Time: %f s\n", end - start);

  return 0;
}
