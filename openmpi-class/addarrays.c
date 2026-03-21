#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// main goal: read in a matrix and add all the entries together
int main(int argc, char* argv[])
{
  // declare variables
  int my_rank, size;
  FILE* file;
  int rows, cols;

  // start communication world
  MPI_Init(&argc, &argv); // could NULL, NULL
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
 

  // rank 0: read in the file
  if (my_rank == 0) 
  {
    file = fopen("matrix.txt", "r");
    if(!file) 
    {
      printf("Error: could not open file\n");
      exit(1);
    }
  
    fscanf(fp, "%d %d", &rows, &cols);
  }
  
  // broadcast rows and cols
  MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // all PE's print received values
  printf("%3d: r-%d c-%d\n", my_rank, rows, cols);

  // scatter the matrix row entries
  // create a flag to signal when to stop
  int flag = 1;
  int* vals = NULL; // row entries for PE 0

  if(my_rank == 0) 
  {
    vals = malloc(cols*sizeof(int));
  }

  int val, my_sum = 0;
  while(flag)
  {
    // PE0: read in one row
    if(my_rank == 0) 
    {
      for(int col = 0; col < cols; col++) 
      {
        fscanf(fp, "%d", &vals[col]);
      }

      if(feof(file)) 
      {
        fclose(file);
        flag = 0;
      }
    }

    // scatter the information
    if(flag)
    {
      MPI_Scatter(vals, 1, MPI_INT,  // send: msg_ptr, msg_size, msg_type
                  &val, 1, MPI_INT,  // recv: dest_prt, dest_size, msg_type
                  0, MPI_COMM_WORLD);// send_tag, community


      my_sum += val;
      printf("%3d: v-%d => s-%d\n", my_rank, val, my_sum);
    }
    
    // update the flag
    // having this after the MPI_Scatter ensures the last row gets sent and processed
    MPI_Bcast(&flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }

  // reduce all of the information
  int tot_sum;
  MPI_Reduce(&my_sum, &tot_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  // end world
  MPI_Finalize(); 

  printf("Total Sum: %d\n", tot_sum);

  return 0;
}
