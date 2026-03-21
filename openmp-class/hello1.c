// hello.c

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef _OPENMP
# include <omp.h>
#endif

void hello() {
#ifdef _OPENMP
  int my_rank = omp_get_thread_num();
  int thread_count = omp_get_num_threads();
#else
  int my_rank = 0;
  int thread_count = 1;
#endif
  printf("%d/%d:\thello\n", my_rank+1, thread_count);
}

int main(int argc, char* argv[]) {

  if(argc != 2) {
    printf("usage: %s <threads>\n", argv[0]);
    exit(1);
  }

  // grab from user threads
  int thread_count = strtol(argv[1], NULL, 10);

  // section to parallelize
# pragma omp parallel num_threads(thread_count)
  hello();

  return 0;
}
