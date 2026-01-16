#include <stdio.h>

#include <hpc-lib/timing/timing.h>

void foo() {
  for(int i = 0; i < 10000; i++) {
    printf("%d\n", i);
  }
}

int main(void) {

  foo();

  foo();

  return 0;
}