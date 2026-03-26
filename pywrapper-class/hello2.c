#include <stdio.h>
#include <stdlib.h>

char* hello(int x) {
  char* greeting = malloc(sizeof(char)*100);
  sprintf(greeting, "Hello %d!", x);
  return greeting;
}

// debug code
int main()
{
  char* greeting = hello(1000);
  printf("%s\n", greeting);
  free(greeting);
  return 0;
}
