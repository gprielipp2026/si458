#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    printf("usage: %s <NUMSTEPS>\n", argv[0]);
    exit(1);
  }

  int rank, size;
  int NUMSTEPS = atoi(argv[1]);
  

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);


  int val = rank;
  int rval;
  int sval;
    
  int partner = ( rank + size / 2) % size; // the one directly across from it

  for(int i = 0; i < NUMSTEPS; i++)
  {
    // shift the valid senders to the left
    int s = (((rank-i) % size) + size) % size; // ensure it's always positive
    if(s < size / 2)
    { 
      //printf("%2d: %2d sending to %2d\n", i, rank, partner);
      sval = (rank + 1) * i;
      MPI_Send(&sval, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
      MPI_Recv(&rval, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      val += rval;
    }
    else 
    {
      //printf("%2d: %2d receiving from %2d\n", i, rank, partner);
      MPI_Recv(&rval, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      val += rval;
      sval = (rank + 1) * i;
      MPI_Send(&sval, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
    }
  }
 
  // send everything to proc 0
  if(rank)
  {
    MPI_Send(&val , 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  else
  {
    printf("%2d: %5d\n", rank, val);
    for(int p = 1; p < size; p++)
    {
      MPI_Recv(&rval, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%2d: %5d\n", p, rval);
    }
  }

  MPI_Finalize();

  return 0;
}
