#include <mpi.h>
#include <omp.h>

#include <immintrin.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

// type info
// should do some check to make sure its available and have a fallback
#define TYPE __m512i

// data definitions
typedef struct {
  // not counting the halo
  // only "workable" cells
  uint32_t rows, cols;

  TYPE **read;
  TYPE **write;
} gol_t;

typedef struct {
  uint32_t gens;
  uint8_t freq;
  uint8_t verb;  
  uint32_t rows, cols;
 
  union {
    uint32_t seed;
    char *filepath[128]; 
  };

  gol_t* (*init_gol)(void*);
} params_t;

typedef struct {

} timing_t;

// function declarations
void swap(gol_t *gol);
void start(timing_t *timing);
void stop(timing_t *timing);
void print_time(timing_t *timing);

params_t* parse_args(int argc, char* argv[]);
void free_params(params_t *params);
void free_gol(gol_t *gol);

// main program
int main(int argc, char* argv[])
{
  uint8_t rank, size, left, right;
  
  params_t *params;
  gol_t *gol;
  timing_t *timing; 

  // init MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size); 
  
  // 1. parse arguments & 2. parse inputs
  if (rank == 0)
  {
    params = parse_args(argc, argv); 
    MPI_Bcast(params, sizeof(params), MPI_TYPE, 0, MPI_COMM_WORLD);
  }
  else
  {

  } 


  
  // print timing information
  
  
  // finalize MPI
  MPI_Finalize();
  return 0;
}

// function definitions
// private
TYPE** init_grid(int rows, int cols)
{

}

gol_t* read_file(void *arg)
{
  param_t *params = (param_t*)arg;
}

gol_t* randomize(void *arg)
{
  param_t *params = (param_t*)arg;

}


// public


