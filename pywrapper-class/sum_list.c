#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

uint16_t sum_list(uint8_t* x)
{
  uint16_t total = 0;
  for(int i = 0; i < sizeof(x)/sizeof(uint8_t); i++)
    total += x[i];

  return total;
}

// debug code
int main()
{
  printf("sum of [1,2,3,4] is %d\n", sum_list((uint8_t[]){1, 2, 3, 4}));
  return 0;
}
