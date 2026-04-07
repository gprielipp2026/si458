/**
 * AI Disclaimer:
 * I am letting it help me try to debug my code.
 */

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
  int my_rank;
  int size;
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

gol_t* parse_args(int argc, char* argv[], int size);
gol_t* send_file(params_t *params, char *filepath, int size);
gol_t* receive_file(params_t *params, int rank);
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
  int rank, size;
  
  params_t *params;
  gol_t *gol;
  
  fprintf(stdout, "[MAIN] Starting application\n");
  fflush(stdout);

  /**
   * I caved and allowed Gemini to help me understand the documentation which
   * did not describe what all of the things meant. I still wrote the code. I do
   * not trust it fully.
   */

  // init MPI
  fprintf(stdout, "[MAIN] Initializing MPI\n");
  fflush(stdout);
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  fprintf(stdout, "[MAIN] MPI initialized - rank=%d, size=%d\n", rank, size);
  fflush(stdout); 
 
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
  fprintf(stdout, "[MAIN] Starting parse_args phase (rank=%d)\n", rank);
  fflush(stdout);
  if (rank == 0)
  {
    // broadcasts: 1) which init function to use. 2) send the seed if necessary / start reading + passing the file
    fprintf(stdout, "[MAIN] Rank 0 calling parse_args\n");
    fflush(stdout);
    gol = parse_args(argc, argv, size);
    fprintf(stdout, "[MAIN] Rank 0 parse_args returned\n");
    fflush(stdout);
  }
  else
  {
    fprintf(stdout, "[MAIN] Rank %d waiting for shutdown broadcast\n", rank);
    fflush(stdout);
    uint8_t shutdown;
    MPI_Bcast(&shutdown, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);
    fprintf(stdout, "[MAIN] Rank %d got shutdown=%d\n", rank, shutdown);
    fflush(stdout);
    if(shutdown) exit(1);

    uint8_t isRandom;
    uint32_t seed;
    // figure out which initialization method to use
    fprintf(stdout, "[MAIN] Rank %d waiting for isRandom broadcast\n", rank);
    fflush(stdout);
    MPI_Bcast(&isRandom, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);
    fprintf(stdout, "[MAIN] Rank %d got isRandom=%d\n", rank, isRandom);
    fflush(stdout);

    if(isRandom) 
    {
      MPI_Recv(&seed, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      srand(seed);
    }

    // receive the rest of the params
    fprintf(stdout, "[MAIN] Rank %d allocating params\n", rank);
    fflush(stdout);
    params = malloc(sizeof(*params));
    fprintf(stdout, "[MAIN] Rank %d waiting for params broadcast\n", rank);
    fflush(stdout);
    MPI_Bcast(params, 1, MPI_PARAM_STRUCT, 0, MPI_COMM_WORLD);
    fprintf(stdout, "[MAIN] Rank %d got params - rows=%d cols=%d\n", rank, params->rows, params->cols);
    fflush(stdout);

    if(isRandom) {
      fprintf(stdout, "[MAIN] Rank %d calling gol_randomize\n", rank);
      fflush(stdout);
      gol = gol_randomize(params);
    } else {
      fprintf(stdout, "[MAIN] Rank %d calling receive_file\n", rank);
      fflush(stdout);
      gol = receive_file(params, rank);
    }
    fprintf(stdout, "[MAIN] Rank %d grid created\n", rank);
    fflush(stdout);
  } 

  // Get params from gol structure (works for both rank 0 and non-0)
  fprintf(stdout, "[MAIN] Rank %d setting params (gol->params=%p)\n", rank, (void*)gol->params);
  fflush(stdout);
  params = gol->params;
  params->lPartner = (((rank - 1) % size) + size) % size;
  params->rPartner = (rank + 1) % size;
  gol->my_rank = rank;
  gol->size = size;
  fprintf(stdout, "[MAIN] Rank %d params set - rows=%d cols=%d lPartner=%d rPartner=%d\n", 
          rank, params->rows, params->cols, params->lPartner, params->rPartner);
  fflush(stdout);

  // simulate stuff
  fprintf(stdout, "[MAIN] Rank %d calling simulate\n", rank);
  fflush(stdout);
  simulate(gol);
  fprintf(stdout, "[MAIN] Rank %d simulate returned\n", rank);
  fflush(stdout);
  
  // print timing information
  
  
  // finalize MPI
  fprintf(stdout, "[MAIN] Rank %d finalizing MPI\n", rank);
  fflush(stdout);
  MPI_Finalize();
  fprintf(stdout, "[MAIN] Rank %d done\n", rank);
  fflush(stdout);
  return 0;
}

// function definitions
// private
// DONE
TYPE** init_grid(int rows, int cols)
{
  fprintf(stdout, "[INIT_GRID] rows=%d cols=%d\n", rows, cols);
  fflush(stdout);
  
  // would be more "efficient" to do both arrays at the same time. 
  // but, this effectively runs in the same "time" (big-O time at least).

  // make space for the halo (+2 all around)
  // Need to store ceil((rows+2) / 2) bits per row in __m512i units
  // Each __m512i holds 512 bits, so we need ceil((rows+2)*2 / 512) blocks = ceil((rows+2) / 256) blocks
  int reduced_rows = ((rows + 2) + 255) / 256;  // Ceiling division: total rows/256
  fprintf(stdout, "[INIT_GRID] reduced_rows=%d (cols+2)=%d\n", reduced_rows, cols+2);
  fflush(stdout);
  
  TYPE** grid = malloc(sizeof(TYPE*) * (cols+2));
  fprintf(stdout, "[INIT_GRID] allocated grid=%p\n", (void*)grid);
  fflush(stdout);
  
  for(int col = 0; col < cols+2; col++)
  {
    grid[col] = calloc(reduced_rows, sizeof(TYPE));
    if(!grid[col]) {
      fprintf(stdout, "[INIT_GRID] ERROR: calloc failed for col=%d\n", col);
      fflush(stdout);
      return NULL;
    }
  }
  fprintf(stdout, "[INIT_GRID] done\n");
  fflush(stdout);

  return grid;
}

typedef struct {
  uint16_t shift;  // Changed from uint8_t to uint16_t to hold values 0-510
  uint32_t block;
  uint32_t col;
} pos_t;

// DONE
pos_t get_pos(uint32_t row, uint32_t col)
{
  // Offset row by 1 to account for halo rows (row -1 to row gol->params->rows)
  // When row = -1 (UINT32_MAX), row_index = 0 (top halo)
  // When row = 0, row_index = 1 (first data row)
  // When row = gol->params->rows, row_index = gol->params->rows+1 (bottom halo)
  uint32_t row_index = row + 1;
  // offset cycles 0-255 every 256 row_indices, so each block holds 256 rows
  uint32_t block  = row_index >> 8;  // row_index / 256
  uint32_t offset = (row_index % (sizeof(TYPE) << 2));  // 0-255
  
  pos_t result = {.shift=offset<<1, .block=block, .col=col+1};
  
  if(row < 1000 && col < 10) {  // Log only first few calls to avoid spam
    fprintf(stdout, "[GET_POS] row=%u col=%u -> block=%u shift=%u col=%u\n", 
            row, col, result.block, result.shift, result.col);
    fflush(stdout);
  }
  
  return result;
}

// public
// DONE
gol_t* send_file(params_t *params, char *filepath, int size)
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
    fscanf(file, "%d %d", &cmd.row, &cmd.col);
    cmd.dest = cmd.row * size / params->rows;
    MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);
  }
  // signal to be done
  cmd.dest = 0;
  MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);

  return gol;
}

// DONE
gol_t* receive_file(params_t *params, int rank)
{
  gol_t* gol = malloc(sizeof(*gol));

  gol->params = params;
  gol->write = init_grid(params->rows, params->cols);
  gol->read = init_grid(params->rows, params->cols);

  cmd_t cmd;
  cmd.dest = 1;
  while(cmd.dest)
  {
    MPI_Bcast(&cmd, 1, MPI_INIT_INFO, 0, MPI_COMM_WORLD);
    if(cmd.dest == rank)
    {
      write(gol, cmd.row, cmd.col, 1);
    }
  }

  return gol;
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
gol_t* parse_args(int argc, char* argv[], int size)
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

  // signal that we're NOT shutting down
  uint8_t shutdown = 0;
  MPI_Bcast(&shutdown, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);

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
    return send_file(params, filepath, size);
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
            // Extract bit from SIMD value using intrinsics
            uint64_t* val_ptr = (uint64_t*)&val;
            printf("%ld ", (val_ptr[idx] >> 0) & 0x01);
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
    TYPE val = _mm512_setzero_si512();
    for(uint32_t row = 0; row < gol->params->rows; row++)
    {
      for(uint32_t col = 0; col < gol->params->cols; col++)
      {
        if(col > 0 && col % (sizeof(TYPE)>>3) == 0)
        {
          MPI_Send(&val, 64, MPI_BYTE, 0, col + 1 == gol->params->cols, MPI_COMM_WORLD);
          val = _mm512_setzero_si512();
        }
        val |= read(gol, row, col) >> (col % (sizeof(TYPE)>>3));
      }

      uint64_t* val_ptr = (uint64_t*)&val;
      if(val_ptr[0] | val_ptr[1] | val_ptr[2] | val_ptr[3] | val_ptr[4] | val_ptr[5] | val_ptr[6] | val_ptr[7])
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
  fprintf(stdout, "[SIMULATE] Starting simulation - rows=%d cols=%d gens=%d\n",
          gol->params->rows, gol->params->cols, gol->params->gens);
  fflush(stdout);

# pragma omp parallel
  {
    int size = omp_get_num_threads();
    int rank = omp_get_thread_num();

    fprintf(stdout, "[SIMULATE] Thread %d/%d starting\n", rank, size);
    fflush(stdout);

    // define the block based on thread's # in the pool
    uint32_t col_start = gol->params->cols * rank / size;
    uint32_t row_start = gol->params->rows * rank / size;
    uint32_t col_end = gol->params->cols * (rank+1) / size;
    uint32_t row_end = gol->params->rows * (rank+1) / size;

    fprintf(stdout, "[SIMULATE] Thread %d: col_start=%u col_end=%u row_start=%u row_end=%u\n",
            rank, col_start, col_end, row_start, row_end);
    fflush(stdout);

    // start the "game"
    for(uint32_t gen = 0; gen < gol->params->gens; gen++)
    {
      fprintf(stdout, "[SIMULATE] Thread %d: generation %u\n", rank, gen);
      fflush(stdout);
      
      // sync all of the threads for work
#     pragma omp barrier

      // update the halo
      fprintf(stdout, "[SIMULATE] Thread %d: updating halo cols %u-%u\n", rank, col_start, col_end);
      fflush(stdout);
      for(uint32_t col = col_start; col < col_end; col++)
      {
        // does the top and bottom along the threads
        write(gol, -1, col, read(gol, gol->params->rows-1, col));
        write(gol, gol->params->rows, col, read(gol, 0, col));
      }
      fprintf(stdout, "[SIMULATE] Thread %d: halo update done\n", rank);
      fflush(stdout);

      // construct the data to send to other nodes
      if(rank == 0) {
        // since I'm storing in column order,
        // I just need to grab and send each outer column segment
        // blocks (# of TYPE's required) * (# bytes per TYPE)
        uint32_t blocks = ((gol->params->rows + 2) + 255) / 256;  // match init_grid calculation
        uint32_t bytes = blocks * sizeof(TYPE);

        uint8_t lHaloBytes[bytes];
        uint8_t rHaloBytes[bytes];

        // Use MPI_Sendrecv to avoid deadlock in ring topology
        MPI_Sendrecv(
            (uint8_t*)gol->read[1], bytes, MPI_BYTE, gol->params->lPartner, 0,
            rHaloBytes, bytes, MPI_BYTE, gol->params->rPartner, 0,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(
            (uint8_t*)gol->read[gol->params->cols], bytes, MPI_BYTE, gol->params->rPartner, 0,
            lHaloBytes, bytes, MPI_BYTE, gol->params->lPartner, 0,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // now I need to put it into my halo
        uint32_t bytesPerBlock = sizeof(TYPE);
        for(uint32_t block = 0; block < blocks; block++)
        {
          memcpy((void*)&gol->write[0][block], lHaloBytes + (block * bytesPerBlock), bytesPerBlock);
          memcpy((void*)&gol->write[gol->params->cols+1][block], rHaloBytes + (block * bytesPerBlock), bytesPerBlock);
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
      // Calculate which block corresponds to row_start
      // Each block holds 256 rows, so: block = row_index / 256
      uint32_t block_start = ((row_start + 1) >> 8);  // (row_start + 1) / 256
      uint32_t current_block = block_start;
      for(uint32_t col = col_start; col < col_end; col++)
      {
        // Compute neighbors for the starting block
        TYPE neighbors = _mm512_add_epi64( _mm512_add_epi64(gol->read[col-1][current_block], gol->read[col][current_block]), gol->read[col+1][current_block] );

        for(uint32_t row = row_start; row < row_end; row++)
        {
          pos_t pos = get_pos(row, col);
          
          // If we've moved to a new block, recompute neighbors for the new block
          if(pos.block != current_block) {
            current_block = pos.block;
            neighbors = _mm512_add_epi64( _mm512_add_epi64(gol->read[col-1][current_block], gol->read[col][current_block]), gol->read[col+1][current_block] );
          }
          
          uint64_t* neighbors_ptr = (uint64_t*)&neighbors;
          uint32_t element_idx = pos.shift / 64;  // Which uint64_t element (0-7)
          uint32_t bit_idx = pos.shift % 64;      // Which bit within that element (0-63)
          uint8_t count = (neighbors_ptr[element_idx] >> bit_idx) & 0x03;
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
  
  if(!gol || !gol->write || !gol->write[pos.col]) {
    fprintf(stdout, "[WRITE] ERROR: Invalid pointers - gol=%p write=%p write[%u]=%p val=%d\n",
            (void*)gol, gol?(void*)gol->write:NULL, pos.col, 
            (gol && gol->write)?(void*)gol->write[pos.col]:NULL, val);
    fflush(stdout);
    return;
  }
  
  uint64_t* write_ptr = (uint64_t*)&gol->write[pos.col][pos.block];
  uint32_t element_idx = pos.shift / 64;
  uint32_t bit_idx = pos.shift % 64;
  
  if(element_idx >= 8) {
    fprintf(stdout, "[WRITE] ERROR: element_idx=%u out of range (max 7), shift=%u\n", element_idx, pos.shift);
    fflush(stdout);
    return;
  }
  
  write_ptr[element_idx] |= ((uint64_t)val) << bit_idx;
}

// DONE
uint8_t read(gol_t *gol, uint32_t row, uint32_t col)
{
  pos_t pos = get_pos(row, col);
  
  if(!gol || !gol->read || !gol->read[pos.col]) {
    fprintf(stdout, "[READ] ERROR: Invalid pointers - gol=%p read=%p read[%u]=%p\n",
            (void*)gol, gol?(void*)gol->read:NULL, pos.col,
            (gol && gol->read)?(void*)gol->read[pos.col]:NULL);
    fflush(stdout);
    return 0;
  }
  
  uint64_t* read_ptr = (uint64_t*)&gol->read[pos.col][pos.block];
  uint32_t element_idx = pos.shift / 64;
  uint32_t bit_idx = pos.shift % 64;
  
  if(element_idx >= 8) {
    fprintf(stdout, "[READ] ERROR: element_idx=%u out of range (max 7), shift=%u\n", element_idx, pos.shift);
    fflush(stdout);
    return 0;
  }
  
  return (read_ptr[element_idx] >> bit_idx) & 0x01;
}

