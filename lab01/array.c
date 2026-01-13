#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  int threshold;
  int maxIters;
  int rows, cols;
  int freq;
  int** mat;
} info_t;

uint64_t get_time_ms(); // get the system time in milliseconds
info_t* parse_args(int argc, char* argv[]);
void  disp_mat(info_t* info);
int** make_array(int rows, int cols);
int   update(info_t* info);
int   unif_rand(int lower, int upper);
void free_info(info_t* info);
void simulate(info_t* info);

int main(int argc, char* argv[]) {
  // parse the arguments
  info_t* info = parse_args(argc, argv); 

  // simulate
  simulate(info);

  // free memory
  free_info(info);

  return 0;
}

uint64_t get_time_ms() {
  // this came from https://stackoverflow.com/questions/10192903/time-in-milliseconds-in-c
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (((uint64_t)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}

void parse_file(char* path, int** mat, int* rows, int* cols) {
    // parse the file into mat
    FILE* file = fopen(path, "r");

    if(!file) {
      printf("issue openening '%s'\n", path);
      exit(1);
    }

    // first row: rows cols inputSize(# of rows remaining)
    int inputSize;
    fscanf(file, "%d %d %d",  rows, cols, &inputSize);
    // read the remaining rows
    // they are formatted in: row col value
    mat = make_array(*rows, *cols);
    while(inputSize--) {
      int row, col, val;
      fscanf(file, "%d %d %d", &row, &col, &val);
      mat[row][col] = val;
    }
}

info_t* parse_args(int argc, char* argv[]) {
  int maxIters, freq, rows, cols;
  int threshold, seed;
  int** mat = NULL;
  

  if(argc != 7 && argc != 5) {
    printf("usage: %s <max iters> <threshold> <display frequency> <seed for rand> <?rows> <?cols>\n", argv[0]);
    exit(1);
  }

  maxIters = atoi(argv[1]);
  threshold = atoi(argv[2]);
  freq = atoi(argv[3]);
  seed = atoi(argv[4]);

  if(argc == 7) {
    rows = atoi(argv[5]);
    cols = atoi(argv[6]);

    mat = make_array(rows, cols);
  } else {
    // (?read in the file)
    printf("Matrix File Path: ");
    char path[100];
    fgets(path, sizeof(path), stdin);
    path[strlen(path)-1] = '\0'; // remove the newline

    parse_file(path, mat, &rows, &cols); 
  }

  // seed random (if seed > 0)
  if(seed >= 0) {
    srand(seed);
  }
  else {
    // get the current time for the seed
    srand( get_time_ms() );
  }

  // create the struct
  info_t* info = malloc(sizeof(*info));
  info->maxIters = maxIters;
  info->mat = mat;
  info->rows = rows; 
  info->cols = cols;
  info->threshold = threshold;
  info->freq = freq;

  return info;
}

void free_info(info_t* info) {
  // free mat
  for(int row = 0; row < info->rows; row++) {
    free(info->mat[row]);
  }
  free(info->mat);
  free(info);
}

void  disp_mat(info_t* info) {
  // loop through and print the matrix
  printf("------------\n");
  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      printf("%d ", info->mat[row][col]);
    }
    printf("\n");
  }
  printf("------------\n");
}


int** make_array(int rows, int cols) {
  int** mat = malloc(sizeof(int*)*rows);

  for(int i = 0; i < rows; i++) {
    mat[i] = malloc(sizeof(int)*cols);
    for(int j = 0; j < cols; j++) {
      mat[i][j] = 0;
    }
  }

  return mat;
}


int   update(info_t* info) {
  int count = 0;

  // check every cell (naive approach)
  for(int row = 0; row < info->rows; row++) {
    for(int col = 0; col < info->cols; col++) {
      // if the point is within [-threshold, threshold] <-- update point
      if(info->mat[row][col] >= -info->threshold && info->mat[row][col] <= info->threshold) {
        info->mat[row][col] += unif_rand(-info->threshold, info->threshold);
        count++;
      }
    }
  }

  return count;
}


int   unif_rand(int lower, int upper) {
  // apply a uniform distribution
  // taken from the class notes
  return (int) ( (upper/2.0 - lower/2.0 + 1.0) * ((double) rand()/(RAND_MAX+1.0)) ) + lower/2;
}

void simulate(info_t* info) {
  int changes = 1;
  for(int iter = 0; iter < info->maxIters && changes > 0; iter++) {
    if (info->freq > 0 && iter % info->freq == 0) {
      printf("count: %d\n", iter);
      disp_mat(info);
    }
    changes = update(info);
  }

  printf("final\n");
  disp_mat(info);
}

