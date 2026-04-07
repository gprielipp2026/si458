#include <stdio.h>
#include <stdlib.h>

// tells nvcc that this is a gpu function
__global__ void mykernel(void) 
{
  // gpu function  

}



int main()
{
  printf("Hello World\n");
 
  // calling gpu code -- run mykernel()
  // use 1 block and 1 thread
  mykernel<<<1,1>>>();

  return 0;
}
