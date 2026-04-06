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
  uint32_t gens;
  uint32_t rows, cols;
  uint32_t lPartner, rPartner;
  uint8_t freq;
  uint8_t verb;  
} params_t;

typedef struct {
  // not counting the halo
  // only "workable" cells
  params_t* params;
  uint32_t my_rank;
  uint32_t size;
  TYPE **read;
  TYPE **write;
} gol_t;

typedef struct {
  uint8_t dest;// allow up to 256 Nodes
  uint32_t row, col;
} cmd_t;

// CUSTOM MPI TYPES
MPI_Datatype MPI_PARAM_STRUCT;
MPI_Datatype MPI_INIT_INFO;

// function declarations
void swap(gol_t *gol);

gol_t* parse_args(int argc, char* argv[], uint32_t size);
gol_t* send_file(params_t *params, char *filepath);
gol_t* receive_file(params_t *params);
gol_t* gol_randomize(params_t *params);
void free_params(params_t *params);
void free_gol(gol_t *gol);

void print(gol_t* gol);
void simulate(gol_t* gol);

void write(gol_t *gol, uint32_t row, uint32_t col, uint8_t val);
uint8_t read(gol_t *gol, uint32_t row, uint32_t col);

// main program
int main(int argc, char* argv[])
{
  uint32_t rank, size;
  
  params_t *params;
  gol_t *gol;

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
      (int[]){5, 2}, // block lengths 
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
    gol = parse_args(argc, argv, size); 
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

    if(isRandom) gol = gol_randomize(params);
    else gol = receive_file(params, rank);
  } 

  params->lPartner = (((rank - 1) % size) + size) % size;
  params->rPartner = (rank + 1) % size;
  gol->my_rank = rank;
  gol->size = size;

  // simulate stuff
  simulate(gol);
  
  // print timing information
  
  
  // finalize MPI
  MPI_Finalize();
  return 0;
}

// function definitions
// private
// DONE
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

// DONE
pos_t get_pos(uint32_t row, uint32_t col)
{
  uint32_t block  = (row / sizeof(TYPE)) >> 3; // >> 3 multiplies sizeof(TYPE) by 8-bits per byte
  uint32_t offset = (row % (sizeof(TYPE) << 2)); // need >> 1 (divide by 2) and multiply by 8-bits per byte

  return (pos_t){.shift=offset<<1, .block=block, .col=col+1};
}

// public
// DONE
gol_t* send_file(params_t *params, char *filepath, uint8_t size)
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

// DONE
gol_t* receive_file(params_t *params, uint8_t rank)
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

// DONE
gol_t* gol_randomize(params_t *params)
{
  gol_t* gol = malloc(sizeof(*gol));
  gol->params = params;
  gol->write = init_grid(params->rows, params->cols);
  gol->read = init_grid(params->rows, params->cols);

  for(uint32_t col = 0; col < params->cols; col++)
  {
    for(uint32_t row = 0; row < params->rows; row++)
    {
      write(gol, row, col, rand() % 2);
    }
  }

  return gol;
}

// DONE
void swap(gol_t *gol)
{
  TYPE** tmp = gol->write;
  gol->write = gol->read;
  gol->read = tmp;
}

// DONE
gol_t* parse_args(int argc, char* argv[], uint32_t size)
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
  // I think this could cause problems from integer division...
  params->rows = rows / size; // how many each node should have
  params->cols = cols / size;
  params->gens = (uint32_t) atoi(argv[1]);
  params->freq = (uint8_t)  atoi(argv[2]);
  params->verb = (uint8_t)  atoi(argv[3]);

  // send the params (send_file will send it otherwise);
  if(isRandom)
    MPI_Bcast(params, 1, MPI_PARAM_STRUCT, 0, MPI_COMM_WORLD);

  // get the gol_t back to rank 0
  if(isRandom)
    return gol_randomize(params);
  else
    // communicate the file if I need to
    return send_file(params, filepath);
}

// DONE
void free_params(params_t *params)
{
  free(params);
}

// DONE
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

// DONE?
void print(gol_t* gol)
{
  if(gol->my_rank == 0)
  {
    // print row by row and recieve each row from the other ranks
    MPI_Status status;
    TYPE val;
    for(uint32_t row = 0; row < gol->params->rows; row++)
    {
      // print my first row
      for(uint32_t col = 0; col < gol->params->cols; col++)
      {
        printf("%d ", read(gol, row, col));
      }

      // read from the other nodes
      for(uint8_t p = 1; p < gol->size; p++)
      {
        status.MPI_TAG = 1;
        while(status.MPI_TAG) { // have it send a 0 when it's done
          MPI_Recv(&val, 64, MPI_BYTE, p, 0, MPI_COMM_WORLD, &status);

          for(uint16_t idx = 0; idx < (sizeof(TYPE)>>3); idx++)
          {
            printf("%d ", (val >> idx) & 0x01);
          }
        }
      }

      // end of row
      printf("\n");
    }

  }
  else 
  {
    // need to send all of my rows
    TYPE val = 0;
    for(uint32_t row = 0; row < gol->params->rows; row++)
    {
      for(uint32_t col = 0; col < gol->params->cols; col++)
      {
        if(col > 0 && col % (sizeof(TYPE)>>3) == 0)
        {
          MPI_Send(&val, 64, MPI_BYTE, 0, col + 1 == gol->cols, MPI_COMM_WORLD);
          val = 0;
        }
        val |= read(gol, row, col) >> (col % (sizeof(TYPE)>>3));
      }

      if(val > 0)
      {
        // make sure everything gets sent
        MPI_Send(&val, 64, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
      }

    }
  }

  // signal that I am done printing
# pragma omp barrier
}

// DONE? 
void simulate(gol_t* gol)
{

# pragma omp parallel
  {
    int size = omp_get_num_threads();
    int rank = omp_get_thread_num();

    // define the block based on thread's # in the pool
    uint32_t col_start = gol->params->cols * rank / size;
    uint32_t row_start = gol->params->rows * rank / size;
    uint32_t col_end = gol->params->cols * (rank+1) / size;
    uint32_t row_end = gol->params->rows * (rank+1) / size;

    // start the "game"
    for(uint32_t gen = 0; gen < gol->params->gens; gen++)
    {
      // sync all of the threads for work
#     pragma omp barrier

      // update the halo
      for(uint32_t col = col_start; col < col_end; col++)
      {
        // does the top and bottom along the threads
        write(gol, -1, col, read(gol, gol->params->rows-1, col));
        write(gol, gol->params->rows, col, read(gol, 0, col));
      }

      // construct the data to send to other nodes
      if(rank == 0) {
        // since I'm storing in column order,
        // I just need to grab and send each outer column segment
        // blocks (# of TYPE's required) * (# bytes per TYPE)
        uint32_t blocks = ((gol->params->rows / sizeof(TYPE)) >> 3);
        uint32_t bytes = blocks * sizeof(TYPE);

        uint8_t lHaloBytes[bytes];
        uint8_t rHaloBytes[bytes];

        // I'm worried this will stall
        MPI_Send(gol->read[0], bytes, MPI_BYTE, gol->params->lPartner, 0, MPI_COMM_WORLD);
        MPI_Recv(rHaloBytes, bytes, MPI_BYTE, gol->params->rPartner, MPI_COMM_WORLD, MPI_IGNORE_STATUS);

        MPI_Send(gol->read[gol->params->rows], bytes, MPI_BYTE, gol->params->rPartner, 0, MPI_COMM_WORLD);
        MPI_Recv(lHaloBytes, bytes, MPI_BYTE, gol->params->lPartner, MPI_COMM_WORLD, MPI_IGNORE_STATUS);

        // now I need to put it into my halo
        uint32_t bytesPerBlock = sizeof(TYPE);
        // for(uint32_t idx = 0; idx < bytes; idx++)
        // {
        //   uint32_t block = idx / bytesPerBlock;
        //   // align shift to the 8 bytes
        //   uint32_t shift = (idx >> 3) % bytesPerBlock;
        //   gol->write[0][block] |= lHaloBytes[idx] >> shift;
        //   gol->write[gol->params->rows][block] |=  rHaloBytes[idx] >> shift;
        // }
        for(uint32_t block = 0; block < blocks; block++)
        {
          memcpy(gol->write[0][block], lHaloBytes + bytesPerBlock, block * bytesPerBlock);
          memcpy(gol->write[gol->params->rows][block], rHaloBytes + bytesPerBlock, block * bytesPerBlock);
        }

#       pragma omp barrier
      } else {
#       pragma omp barrier
      }


      // swap read and write
      swap(gol);

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

      // loop over and update the array
      for(uint32_t col = col_start; col < col_end; col++)
      {
        TYPE neighbors = _mm512_add_epi64( _mm512_add_epi64(gol->read[col-1], gol->read[col]), gol->read[col+1] );

        for(uint32_t row = row_start; row < row_end; row++)
        {
          pos_t pos = get_pos(row, col);
          uint8_t count = (neighbors >> pos.shift) & 0x03;
          uint8_t alive = read(gol, row, col) << 2; // either 0x02 or 0x00

          // alive & count == 2 or count == 3
          if(alive & count) write(gol, row, col, 1);
          else if (!alive && count == 3) write(gol, row, col, 1);
          else write(gol, row, col, 0);
        }
      }

    } // end for
  }// end parallel

}// end simulate

// DONE
void write(gol_t *gol, uint32_t row, uint32_t col, uint8_t val) 
{
  pos_t pos = get_pos(row, col);
  gol->write[pos.col][pos.block] |= val << pos.shift;
}

// DONE
uint8_t read(gol_t *gol, uint32_t row, uint32_t col)
{
  pos_t pos = get_pos(row, col);
  return (uint8_t) ((gol->read[pos.col][pos.block] >> pos.shift) & 0x01);
}

