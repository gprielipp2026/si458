#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

int main(int argc, char* argv[])
{
  // declare variables
  int rank, size;
  int total = 0;

  // start MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // create the hello world messages
  char smsg[64];
  int imsg;
  if(rank)
  {
    // proc >0
    snprintf(smsg, sizeof(smsg), "Hello from process %d of %d", rank, size);
    imsg = rank * 2;
    // Send the message
    MPI_Send(smsg, strlen(smsg) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&imsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  else 
  {
    // proc 0
    printf("Hello from process %d of %d\n", rank, size);
    for(int other = 1; other < size; other++)
    {
      MPI_Recv(smsg, sizeof(smsg), MPI_CHAR, other, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&imsg, 1, MPI_INT, other, 0 , MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      printf("%s\n", smsg);
      total += imsg;
    }
  }

  if(rank == 0) 
  {
    printf("Combined values sent: %d\n", total);
  }

  // Finalize
  MPI_Finalize();

  return 0;
}
