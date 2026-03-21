#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <mpi.h>


// from here on out, everything is in parallel and the PEs are doing stuff
int main() 
{
  // find out the number of PE and rank of PE
  int comm_sz; // community size
  int my_rank; 

  // initialize mpi and allocate resources
  // creating the communication network
  MPI_Init(NULL, NULL); // &argc, &argv if needed

  // each process receives information on size and rank
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // create greeting
  char greeting[100];

  if(my_rank != 0) {
    sprintf(greeting, "hello from %d of %d\n", my_rank, comm_sz);

    //      content,   length,               type,    dest, send_tag, community
    MPI_Send(greeting, strlen(greeting) + 1, MPI_CHAR, 0,   0,         MPI_COMM_WORLD);
  } else {
    // print my own message to screen:
    printf("I am from process 0!\n");

    // recv all messages and print them
    for(int p = 1; p < comm_sz; p++) 
    {
      //       address, max size, type,    source, recv_tag, community, &status
      MPI_Recv(greeting, 100,     MPI_CHAR, p,     0,        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%s\n", greeting);
    }
  }

  // clean up resources
  MPI_Finalize();

  return 0;
}
