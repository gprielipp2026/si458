#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

uint16_t sum(uint8_t x, uint8_t y)
{
  return x + y;
}

// debug code
int main()
{
  printf("sum of 10 and 12 is %d\n", sum(10, 12));
  return 0;
}
