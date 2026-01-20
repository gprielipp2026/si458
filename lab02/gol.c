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
#include <stdint.h>
#include <string.h>
#include <stdbool.h> 

typedef struct {
  int gens;
  int rows, cols;
  int freq;
  int** mat;
} info_t;

info_t* parse_args(int argc, char* argv[]);
void free_info(info_t* info);
void simulate(info_t* info);

// -------------------------- main ------------------------

int main(int argc, char* argv[]) {
  info_t* info = parse_args(argc, argv);
  simulate(info);
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

pos_t pindex(info_t* info, int x, int y) {
  return (pos_t){ x + 1, y + 1 };
}


void  disp_mat(info_t* info) {
  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      pos_t pos = pindex(info, col, row);
      printf("%d ", info->mat[pos.y][pos.x]);
    }
    printf("\n");
  }
}

void make_array(info_t* info, bool useRand) {
  // create it as [rows] x [cols]

  info->mat = malloc(sizeof(int)*(info->rows+2));
  for(int row = 0; row < info->rows+2; row++) {
    info->mat[row] = malloc(sizeof(int)*(info->cols+2));
    for(int col = 0; col < info->cols+2; col++) {
      if(useRand) info->mat[row][col] = rand() % 2;
      else info->mat[row][col] = 0;
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
      pos_t pos = pindex(row, col);
      info->mat[pos.y][pos.x] = 1;
    }
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
    srand(atoi(argv[3]));
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
   info->mat[0][col] = info->mat[info->rows][col];
   info->mat[info->rows+1][col] = info->mat[1][col];
 }

 // do the left and right columns
 for(int row = 1; row < info->rows+1; row++) { 
  info->mat[row][0] = info->mat[row][info->cols];
  info->mat[row][info->cols+1] = info->mat[row][1];
 }
}

int count_neighbors(info_t* info, int x, int y) {
  pos_t pos = pindex(info, x, y);

  int count = 0;
  for(int yoff = -1; yoff <= 1; yoff++) {
    for(int xoff = -1; xoff <= 1; xoff++) {
      if(yoff == 0 && xoff == 0) continue;
      // also highly memory inefficient
      count += info->mat[pos.y + yoff][pos.x + xoff];
    }
  }

  return count;
}

void update(info_t* info) {
  update_halo(info);

  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      int count = count_neighbors(info, col, row);
      pos_t pos = pindex(info, col, row);
      bool alive = info->mat[pos.y][pos.x];
      
      if(! (alive && (count == 2 || count == 3)) ) info->mat[pos.y][pos.x] = 0;
      else if(!alive && count == 3) info->mat[pos.y][pos.x] = 1;
      else info->mat[pos.y][pos.x] = 0;
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
  printf("final\n--------------------\n");
  disp_mat(info);
  printf("--------------------\n");
}

// ------------------------------- end simulation -------------------------

void free_info(info_t* info) {
  // free mat
  if(info == NULL) return;

  for(int row = 0; row < info->rows+2; row++) {
    free(info->mat[row]);
  }
  free(info->mat);
  free(info);
}
