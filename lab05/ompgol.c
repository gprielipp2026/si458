#include <stdio.h>
#include <stdlib.h>

/**
1. Parse the arguments into a struct
  1. Required in argv: <# gens to simulate> <freq of grid display>
  2. Optional: <seed for rand> <rows (N)> <cols (M)>
    1. If not in argv, get file from stdin
2. Create the matrix (N+2)x(M+2)
3. Initialize matrix
  1. from txt file: 
    1. N M InputSize
    2. X Y
  2. all args used:
    1. every entry = rand() % 2
4. Run the simulation:
  1. update the halo
  2. update the grid:
    1. Any live cell with two or three neighbors survives.
    2. Any dead cell with three live neighbors becomes a live cell.
    3. All other live cells die in the next generation. Similarly, all other dead cells stay dead.
 */

#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h> 
#include <omp.h>

typedef struct {
  int gens;
  int rows, cols;
  int freq;
  int** write;
  int** read;
} info_t;

typedef struct {
    clock_t cstart, cend;
    struct timeval wstart, wend;
    struct {
        int s, us;
    } cdiff, wdiff;
} tinfo_t;

info_t* parse_args(int argc, char* argv[]);
void free_info(info_t* info);
void simulate(info_t* info);


void time_start(tinfo_t* tinfo);
void time_stop(tinfo_t* tinfo);
void time_diff(tinfo_t* tinfo);
void time_print(tinfo_t* tinfo);

// -------------------------- main ------------------------

int main(int argc, char* argv[]) {
  info_t* info = parse_args(argc, argv);
  
  tinfo_t timing;
  time_start(&timing);

  simulate(info);

  time_stop(&timing);
  
  time_diff(&timing);
  time_print(&timing);

  free_info(info);

  return 0;
}

// -------------------------- end of main --------------------

typedef struct {
  int x, y;
} pos_t;



uint64_t get_time_ms() {
  // this came from https://stackoverflow.com/questions/10192903/time-in-milliseconds-in-c
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (((uint64_t)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}


pos_t pindex(int x, int y) {
  return (pos_t){ x + 1, y + 1 };
}



void  disp_mat(info_t* info) {
  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      pos_t pos = pindex(col, row);
      printf("%d ", info->read[pos.y][pos.x]);
    }
    printf("\n");
  }
}


void make_array(info_t* info, bool useRand) {
  // create it as [rows] x [cols]

  info->write = malloc(sizeof(int*)*(info->rows+2));
  info->read = malloc(sizeof(int*)*(info->rows+2));
  for(int row = 0; row < info->rows+2; row++) {
    info->write[row] = malloc(sizeof(int)*(info->cols+2));
    info->read[row] = calloc(sizeof(int), (info->cols+2));
    for(int col = 0; col < info->cols+2; col++) {
      if(useRand) info->write[row][col] = rand() % 2;
      else info->write[row][col] = 0;
    }
  }
}


void parse_file(char* path, info_t* info) {
  FILE* file = fopen(path, "r");

  if(!file) {
    fprintf(stderr, "Could not open '%s'\n", path);
    free_info(info);
    exit(1);
  }

    // first row: rows cols inputSize(# of rows remaining)
    int inputSize;
    fscanf(file, "%d %d %d",  &info->rows, &info->cols, &inputSize);
    // read the remaining rows
    // they are formatted in: row col value
    make_array(info, false);
    while(inputSize--) {
      int row, col;
      fscanf(file, "%d %d", &row, &col);
      pos_t pos = pindex(col, row);
      info->write[pos.y][pos.x] = 1;
    }

  fclose(file);
}


info_t* parse_args(int argc, char* argv[]) {
  if(argc != 3 && argc != 6) {
    fprintf(stderr, "usage: %s <# generations> <display frequency> <?rand seed> <?rows> <?cols>\n", argv[0]);
    exit(1);
  }
  
  info_t* info = malloc(sizeof(*info));
  
  info->gens = atoi(argv[1]);
  info->freq = atoi(argv[2]);

  if(argc == 6) {
    int seed = atoi(argv[3]);

    if(seed < 0) srand(get_time_ms());
    else srand(seed);

    info->rows = atoi(argv[4]);
    info->cols = atoi(argv[5]);
    make_array(info, true);
  } else {
    printf("File path: ");
    char path[100];
    fgets(path, sizeof(path), stdin);
    path[strlen(path)-1] = '\0';
    parse_file(path, info);
  }

  return info;
}

// --------------------------- simulation ---------------------------


void update_halo(info_t* info) {
 // this is going to be inefficient at first
 
 // do the top and bottom rows
 for(int col = 1; col < info->cols+1; col++) { 
   info->write[0][col] = info->write[info->rows][col];
   info->write[info->rows+1][col] = info->write[1][col];
 }

 // do the left and right columns
 for(int row = 1; row < info->rows+1; row++) { 
  info->write[row][0] = info->write[row][info->cols];
  info->write[row][info->cols+1] = info->write[row][1];
 }
}

int count_neighbors(int** mat, int x, int y) {
  pos_t pos = pindex(x, y);

  int count = 0;
  for(int yoff = -1; yoff <= 1; yoff++) {
    for(int xoff = -1; xoff <= 1; xoff++) {
      if(yoff == 0 && xoff == 0) continue;
      // also highly memory inefficient
      count += mat[pos.y + yoff][pos.x + xoff];
    }
  }

  return count;
}

void swap(info_t* info) {
  int** tmp = info->write;
  info->write = info->read;
  info->read = tmp;
}

void update(info_t* info) {
  update_halo(info);
  
  swap(info);

  # pragma omp parallel for schedule(dynamic)
  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      int count = count_neighbors(info->read, col, row);
      pos_t pos = pindex(col, row);
      bool alive = info->read[pos.y][pos.x];
      
      if(alive && (count == 2 || count == 3)) info->write[pos.y][pos.x] = 1;
      else if(!alive && count == 3) info->write[pos.y][pos.x] = 1;
      else info->write[pos.y][pos.x] = 0;
    }
  }
}



void simulate(info_t* info) {
  for(int gen = 0; gen < info->gens; gen++) {
    if(info->freq > 0 && gen % info->freq == 0) {
      printf("%d\n--------------------\n",gen);
      disp_mat(info);
      printf("--------------------\n");
    }

    update(info);
  }
  if(info->freq > 0) {
    printf("final\n--------------------\n");
    disp_mat(info);
    printf("--------------------\n");
  }
}

// ------------------------------- end simulation -------------------------


void free_info(info_t* info) {
  // free mat
  if(info == NULL) return;

  for(int row = 0; row < info->rows+2; row++) {
    free(info->write[row]);
    free(info->read[row]);
  }
  free(info->write);
  free(info->read);
  free(info);
}

void time_start(tinfo_t* tinfo)
{
    tinfo->cstart = clock();
    gettimeofday(&tinfo->wstart, NULL);
}

void time_stop(tinfo_t* tinfo)
{
    tinfo->cend = clock();
    gettimeofday(&tinfo->wend, NULL);
}

void time_diff(tinfo_t* tinfo)
{
    // clock cycles
    tinfo->cdiff.us  = (tinfo->cend - tinfo->cstart) * 1000.0 / CLOCKS_PER_SEC;
    tinfo->cdiff.s   = tinfo->cdiff.us / 1000;
    tinfo->cdiff.us -= 1000 * tinfo->cdiff.s;

    // wall time
    tinfo->wdiff.us  = tinfo->wend.tv_usec - tinfo->wstart.tv_usec;
    tinfo->wdiff.s   = tinfo->wend.tv_sec  - tinfo->wstart.tv_sec + (tinfo->wdiff.us > 1000000);
    tinfo->wdiff.us -= (tinfo->wdiff.us > 1000000) ? 1000000 : 0;
}

void time_print(tinfo_t* tinfo)
{
    printf("cpu    Time: %4.6f sec\n", tinfo->cdiff.s + (float)tinfo->cdiff.us/1000.0);
    printf("wall   Time: %4.6f sec\n", tinfo->wdiff.s + (float)tinfo->wdiff.us/1000000.0);
}