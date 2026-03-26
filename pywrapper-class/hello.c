#include <stdio.h>
#include <stdlib.h>

char* hello() {
  char* greeting = malloc(sizeof(char)*100);
  sprintf(greeting, "Hello world!");
  return greeting;
}

// debug code
int main()
{
  char* greeting = hello();
  printf("%s\n", greeting);
  free(greeting);
  return 0;
}
