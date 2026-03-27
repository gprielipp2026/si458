#include <mpi.h>
#include <omp.h>

#include <immintrin.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

// type info
// should do some check to make sure its available and have a fallback
#define TYPE __m512i

// data definitions
typedef struct {
  uint32_t rows, cols;
  uint32_t gens;
  uint8_t freq;
  uint8_t verb;  
} params_t;

typedef struct {
  // not counting the halo
  // only "workable" cells
  params_t* params;

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
    char *filepath[16]; 
  };

  gol_t* (*init_gol)(void*);
} params_t;

typedef struct {

} timing_t;


typedef struct {
  uint8_t dest;// allow up to 256 Nodes
  uint32_t row, col;
} cmd_t;

// CUSTOM MPI TYPES
MPI_Datatype MPI_PARAM_STRUCT;
MPI_Datatype MPI_INIT_INFO;

// function declarations
void swap(gol_t *gol);
void start(timing_t *timing);
void stop(timing_t *timing);
void print_time(timing_t *timing);

gol_t* parse_args(int argc, char* argv[]);
gol_t* send_file(param_t *params, char *filepath);
gol_t* receive_file(param_t *params);
gol_t* randomize(param_t *params);
void free_params(params_t *params);
void free_gol(gol_t *gol);

void print(gol_t* gol);
void simulate(gol_t* gol);

void write(gol_t *gol, uint32_t row, uint32_t col, uint8_t val);
uint8_t read(gol_t *gol, uint32_t row, uint32_t col);

// main program
int main(int argc, char* argv[])
{
  uint8_t rank, size, left, right;
  
  params_t *params;
  gol_t *gol;
  timing_t *timing; 

  /**
   * I caved and allowed Gemini to help me understand the documentation which
   * did not describe what all of the things meant. I still wrote the code. I do
   * not trust it fully.
   */

  // init MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size); 
 
  // Create the MPI_PARAM_STRUCT type;                  
  MPI_Type_create_struct(
      2, // count of unique blocks
      (int[]){3, 2}, // block lengths 
      (MPI_Aint[]){offsetof(params_t, rows), offsetof(params_t, freq)}, // displacements
      (MPI_Datatype[]){MPI_UINT32_T, MPI_UINT8_T}, // datatypes
      &MPI_PARAM_STRUCT); // output (new datatype)
  // make datatype communicatable
  MPI_Type_commit(&MPI_PARAM_STRUCT);

  // create the struct for passing file info
  MPI_Type_create_struct(
      2,
      (int[]){1, 2},
      (MPI_Aint[]){offsetof(cmd_t, dest), offsetof(cmd_t, row)},
      (MPI_Datatype[]){MPI_UINT8_T, MPI_UINT32_T},
      &MPI_INIT_INFO);
  MPI_Type_commit(&MPI_INIT_INFO); 

  // 1. parse arguments & 2. parse inputs
  if (rank == 0)
  {
    // broadcasts: 1) which init function to use. 2) send the seed if necessary / start reading + passing the file
    gol = parse_args(argc, argv); 
  }
  else
  {
    uint8_t shutdown;
    MPI_Bcast(&shutdown, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);
    if(shutdown) exit(1);

    uint8_t isRandom;
    uint32_t seed;
    // figure out which initialization method to use
    MPI_Bcast(&isRandom, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    if(isRandom) 
    {
      MPI_Recv(&seed, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      srand(seed);
    }

    // receive the rest of the params
    MPI_Bcast(params, 1, MPI_PARAM_STRUCT, 0, MPI_COMM_WORLD);

    if(isRandom) gol = randomize(params);
    else gol = receive_file(params, rank);
  } 

  // simulate stuff
  simulate(gol);
  
  // print timing information
  
  
  // finalize MPI
  MPI_Finalize();
  return 0;
}

// function definitions
// private
TYPE** init_grid(int rows, int cols)
{
  // would be more "efficient" to do both arrays at the same time. 
  // but, this effectively runs in the same "time" (big-O time at least).

  // make space for the halo (+2 all around)
  int reduced_rows = (rows+2) / (sizeof(TYPE) / 2);
  TYPE** grid = malloc(sizeof(TYPE*) * (cols+2));
  
  for(int col = 0; col < cols+2; col++)
  {
    grid[col] = calloc(reduced_rows, sizeof(TYPE));
  }

  return grid;
}

typedef struct {
  uint8_t shift;
  uint32_t block;
  uint32_t col;
} pos_t;

pos_t get_pos(uint32_t row, uint32_t col)
{
  uint32_t block  = (row / sizeof(TYPE)) >> 3; // >> 3 multiplies sizeof(TYPE) by 8-bits per byte
  uint32_t offset = (row % (sizeof(TYPE) << 2)); // need >> 1 (divide by 2) and multiply by 8-bits per byte

  return (pos_t){.shift=offset<<1, .block=block, .col=col+1};
}

// public
gol_t* send_file(param_t *params, char *filepath, uint8_t size)
{
  // TODO I should check if this succeeds
  FILE* file = fopen(filepath, "r");

  int N;
  // first line: rows cols N
  fscanf(file, "%d %d %d", &params->rows, &params->cols, &N);
  MPI_Bcast(params, 1, MPI_PARAM_STRUCT, 0, MPI_COMM_WORLD);

  gol_t* gol = malloc(sizeof(*gol));

  gol->params = params;
  gol->write = init_grid(params->rows, params->cols);
  gol->read = init_grid(params->rows, params->cols);

  cmd_t cmd;
  while(N--)
  {
    fscanf(file, "%d %d", &cmd->row, &cmd->col);
    cmd->dest = cmd->row * size / params->rows;
    MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);
  }
  // signal to be done
  cmd->dest = 0;
  MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);
}

gol_t* receive_file(param_t *params, uint8_t rank)
{
  gol_t* gol = malloc(sizeof(*gol));

  gol->params = params;
  gol->write = init_grid(params->rows, params->cols);
  gol->read = init_grid(params->rows, params->cols);

  cmd_t cmd;
  cmd->dest = 1;
  while(cmd->dest)
  {
    MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);
    if(cmd->dest == rank)
    {
      write(gol, cmd->row, cmd->col, 1);
    }
  }
}

gol_t* randomize(param_t *params)
{
  gol_t* gol = malloc(sizeof(*gol));
  gol->params = params;
  gol->write = init_grid(params->rows, params->cols);
  gol->read = init_grid(params->rows, params->cols);

  for(uint32_t col = 0; col < params->cols; col++)
  {
    for(uint32_t row = 0; row < params->rows; row++)
    {
      pos_t pos = get_pos(row, col);
      gol->write[pos.col][pos.block] |= (rand() % 2) << pos.shift;
    }
  }

  return gol;
}

void swap(gol_t *gol)
{
  TYPE** tmp = gol->write;
  gol->write = gol->read;
  gol->read = tmp;
}

void start(timing_t *timing);
void stop(timing_t *timing);
void print_time(timing_t *timing);

gol_t* parse_args(int argc, char* argv[], uint8_t size)
{
  // only running on rank0
  if(argc != 4)
  {
    printf("usage: %s <gens> <freq> <verb>\n", argv[0]);
    uint8_t shutdown = 1;
    // tell everyone to exit
    MPI_Bcast(&shutdown, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);
    exit(1);
  }

  uint32_t rows=0, cols=0, seed;
  char filepath[128];
  printf("rows: ");
  scanf("%d", &rows);

  uint8_t isRandom;
  if(rows > 0) 
  {
    printf("cols: ");
    scanf("%d", &cols);
    printf("seed: ");
    scanf("%d", &seed);

    if(seed < 0) seed = time(NULL);

    isRandom = 1;
  }
  else 
  {   
    printf("filename: ");
    fgets(filepath, sizeof(filepath), stdin);
    filepath[strlen(filepath)-1] = '\0';

    isRandom = 0;
  } 

  // let the rest know how to init
  MPI_Bcast(&isRandom, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);

  if(isRandom)
  {
    srand(seed);
    for(int p = 1; p < size; p++)
    {
      seed = seed + 2026 * p;
      MPI_Send(&seed, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
    }
  }

  // create the params
  params_t* params = malloc(sizeof(*params));
  params->rows = rows;
  params->cols = cols;
  params->gens = (uint32_t) atoi(argv[1]);
  params->freq = (uint8_t)  atoi(argv[2]);
  params->verb = (uint8_t)  atoi(argv[3]);

  // send the params (send_file will send it otherwise);
  if(isRandom)
    MPI_Bcast(params, 1, MPI_PARAM_STRUCT, 0, MPI_COMM_WORLD);

  // get the gol_t back to rank 0
  if(isRandom)
    return randomize(params);
  else
    // communicate the file if I need to
    return send_file(params, filepath);
}

void free_params(params_t *params)
{
  free(params);
}

void free_gol(gol_t *gol)
{
  for(int col = 0; col < gol->params->cols+2; col++)
  {
    free(gol->read[col]);
    free(gol->write[col]);
  }

  free_params(gol->params);
  free(gol->read);
  free(gol->write);
  free(gol);
}

void print(gol_t* gol)
{


  // signal that I am done printing
# pragma omp barrier
}

void simulate(gol_t* gol)
{

# pragma omp parallel
  {
    int size = omp_get_num_threads();
    int rank = omp_get_thread_num();

    // define the block based on thread's # in the pool
    uint32_t col_start = ...;
    uint32_t row_start = ...;
    uint32_t col_end = ...;
    uint32_t row_end = ...;

    // start the "game"
    for(uint32_t gen = 0; gen < gol->params->gens; gen++)
    {
      // sync all of the threads for work
#     pragma omp barrier

      // update the halo
      for(uint32_t col = col_start; col < col_end; col++)
      {
        pos_t top_row = get_pos(0, col);
        pos_t bot_row = get_pos(gol->params->rows-1, col);

        gol->write[
      }


      if(gol->params->freq > 0 && gen % gol->params->freq == 0) 
      {
        // printing needs to signal all threads when it's done with omp barrier
        if(rank == 0) print(gol);
        else 
        {
          // sync all threads for printing to be done
#         pragma omp barrier
        }
      }

      

    } // end for
  }// end parallel

}// end simulate



