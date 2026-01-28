#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>

/**
outline:

1. parse input (all ints):
 1. max number of iterations
 2. threshold (value inside [-threshold, threshold] eligible for updating)
 3. display frequency
 4. seed for rand
   1. seed < 0 ==> use time(null)
 5. number of threads (>0)
   1. ie: it's specifying extra threads to create
 6. verbosity for debugging
   1. =2 ==> time it
     1. use timer.h & struct clock_t
     - format: Time: # sec #.# millisec
   2. >0 ==> each thread prints:
     1. logical thread ID
     2. start & end column index value
     3. total number of columns it's allocated
     - format: tid #   columns:   #:#   (#)
2. initialize the grid:
 1. ask user for rows (num rows: ):
   1. rows = 0 ==> use file (filename: )
     1. file in format:
     <rows> <cols> <num entries>
     <row> <col> <val>
     ...
   2. rows > 0 ==> ask for cols (num cols: )
 2. if not from file:
   1. set matrix to all zeros
3. create pthread_barrier for synchronization
4. create all threads, store in array on stack
5. run all threads
 1. loop over all iterations
 2. if need to display matrix:
   1. print thread displays matrix, pauses all other threads until it's done
   - print thread will be the main thread
 3. update a cell as needed:
   1. loop over every cell assigned to this thread
   2. if cell within  [-threshold, threshold]:
     1. cell += rand_num(range)
   3. if no updates occurred, terminate thread
 4. wait for all threads to finish one clock cycle before continuing loop
6. wait for all threads to terminate
7. free all allocated memory

 */

// -------------------- data ---------------------------
// information to time the whole program
typedef struct {
  clock_t start, end;
} timing_t;

// information for setup, from the argv
typedef struct {
  int max_iters,
      threshold,
      freq,
      seed,
      threads,
      verbosity;
} info_t;

// information the program is computing
typedef struct {
  info_t* info;
  
  int countDone;  
  int curIter;

  int rows, cols;
  int** mat;
} data_t;

// information a thread needs to run
typedef struct {
  int start, stop; // stop is exclusive
  data_t* data;
} tinfo_t;

// barriers for controlling the threads
// need to synchronize when printing
// and when done printing to begin work
pthread_barrier_t bprint, bwork; 
// extra synchronization for terminating the program
sem_t mutex;

// -----------------------------------------------------





// ----------------------- functions -------------------
info_t* parse_args(int argc, char* argv[]);
data_t* initialize(info_t* info);
void manage_threads(info_t* info, data_t* data);
void free_info(info_t* info);
void free_data(data_t* data);
void free_tinfo(tinfo_t* tinfo);
// -----------------------------------------------------





// ------------------------- main -----------------------

int main(int argc, char* argv[]) {
  info_t* info = parse_args(argc, argv);
  data_t* data = initialize(info);
  manage_threads(info, data);
  free_info(info);
  free_data(data);
  return 0;
}

// ------------------------------------------------------




// --------------------- definitions --------------------

// ----------------------- private ----------------------
int unif_rand(int x) {
  // return rand() within [-x, x];
  return (2.0*(x+1) * (rand() / (RAND_MAX+1.0))) - x;
}

// threaded function (only main going to run this; ie - not really "threaded")
void print(data_t* data)
{
  bool cond = true;
  while(cond) {
    // update the condition after everyone has done work 
    sem_wait(&mutex);
    cond = data->countDone < data->info->threads && data->curIter < data->info->max_iters; 
    sem_post(&mutex);

    if(!cond) break; // there is probably a better way to do this

    pthread_barrier_wait(&bprint); // wait for everyone so I can do work
    
    if(data->info->freq > 0 && data->curIter % data->info->freq == 0) {

      printf("\n%d\n----------------------------------\n", data->curIter);
      for(int col = 0; col < data->cols; col++) {
        for(int row = 0; row < data->rows; row++) {
          printf("%d ", data->mat[col][row]);
        }
        printf("\n");
      }
      printf("----------------------------------\n");
    }
    // signal that I'm done
    data->curIter++;
    pthread_barrier_wait(&bwork);

  }
  printf("\nfinal\n----------------------------------\n");
  for(int col = 0; col < data->cols; col++) {
    for(int row = 0; row < data->rows; row++) {
      printf("%d ", data->mat[col][row]);
    }
    printf("\n");
  }
  printf("----------------------------------\n");

}

// threaded function
void* simulate(void* arg)
{ 
  tinfo_t* tinfo = (tinfo_t*)arg;
  data_t* data = tinfo->data;

  bool done = false;
  bool cond = true;
  while(cond) {
    // print increases the curIter (don't have to worry about it)
    pthread_barrier_wait(&bprint); 
    pthread_barrier_wait(&bwork);

    sem_wait(&mutex);
    cond = data->countDone < data->info->threads; 
    sem_post(&mutex);

    if(!done) {
      // update the matrix
      // bad code (should refactor so there is less)
      int count = 0;
      for(int col = tinfo->start; col < tinfo->stop; col++) {
        for(int row = 0; row < data->rows; row++) {
          int el = data->mat[col][row];
          if(el >= -data->info->threshold && el <= data->info->threshold) {
            count++;
            data->mat[col][row] += unif_rand(data->info->threshold);
          }
        }
      }

      if(count == 0 || data->curIter + 1 == data->info->max_iters) {
        sem_wait(&mutex);
        data->countDone++;
        sem_post(&mutex); 
        done = true;
      }
    }
  }

  pthread_exit(0);
}

// initialize the matrix
void init_mat(data_t* data) 
{
  data->mat = malloc(sizeof(int*)*data->cols);
  for(int col = 0; col < data->cols; col++) {
    data->mat[col] = calloc(data->rows, sizeof(int));
  }
}

// read the matrix in from a file
data_t* read_file(char* path)
{
  FILE* file = fopen(path, "r");

  if(file == NULL) {
    fprintf(stderr, "Error opening '%s'\n", path);
    return NULL;
  }

  data_t* data = malloc(sizeof(*data));

  int numEntries;
  fscanf(file, "%d %d %d", &data->rows, &data->cols, &numEntries);

  init_mat(data);

  int row, col, val;
  while(numEntries--) {
    fscanf(file, "%d %d %d", &row, &col, &val);
    data->mat[col][row] = val;
  }

  fclose(file);

  return data;
}

// --------------------- public -------------------------
info_t* parse_args(int argc, char* argv[])
{
  if(argc != 7) {
    fprintf(stderr, "usage: %s <max iteration> <threshold> <display frequency> <seed for rand; <0 => use time> <threads >0> <verbosity; 2=timing; >0=thread info>\n", argv[0]);
    exit(0);
  }

  info_t* info = malloc(sizeof(*info));

  info->max_iters = atoi(argv[1]);
  info->threshold = atoi(argv[2]);
  info->freq      = atoi(argv[3]);
  info->seed      = atoi(argv[4]);
  info->threads   = atoi(argv[5]);
  info->verbosity = atoi(argv[6]);

  return info;
}

data_t* initialize(info_t* info)
{
  /**
   * 0. set up rand
   * 1. get rows from user
   * 2. if rows == 0:
   *  1. initialize from file
   * 3. else:
   *  1. get cols from user
   * 4. set up pthreads + mutex
   */
  if(info->seed > 0) srand(info->seed);
  else srand(time(NULL));

  data_t* data;

  int rows, cols;
  printf("num rows: ");
  scanf("%d", &rows);

  if(rows == 0) {
    printf("filename: ");
    char path[100];
    fgets(path, sizeof(path), stdin);
    path[strlen(path)-1] = '\0';
    data = read_file(path);

    if(data == NULL) {
      free_info(info);
      exit(0);
    }
  } else {
    printf("num cols: ");
    scanf("%d", &cols);

    data = malloc(sizeof(*data));
    data->rows = rows;
    data->cols = cols;

    init_mat(data); 
  }

  data->info      = info;
  data->countDone = 0;  
  data->curIter   = 0;  

  sem_init(&mutex, 0, 1);
  pthread_barrier_init(&bprint, NULL, data->info->threads + 1);
  pthread_barrier_init(&bwork , NULL, data->info->threads + 1);

  return data;
}


void manage_threads(info_t* info, data_t* data)
{
  // create timing information
  timing_t timing = {
    .start = clock(),
    .end   = 0
  };

  pthread_t threads[info->threads];
  tinfo_t   tinfos [info->threads];

  // create the threads
  int workPerThread = data->cols / info->threads;
  int leftovers = data->cols % info->threads;
  int start = 0, stop;

  for(int t = 0; t < info->threads; t++) {
    // spread the leftover work over the threads
    // at most adds 1 extra task per thread
    stop = start + workPerThread + ( leftovers-- > 0 ? 1 : 0 );

    // create the thread data
    tinfos[t] = (tinfo_t){
      .start   = start,
        .stop    = stop,
        .data    = data
    };

    // if verbose
    if(info->verbosity > 0) {
      printf("tid %d\tcolumns:\t%d:%d\t(%d)\n", t, tinfos[t].start, tinfos[t].stop, tinfos[t].stop - tinfos[t].start);
    }

    pthread_create(&threads[t], NULL, simulate, (void*)&tinfos[t]);

    start = stop;
  }  

  // start the main thread's work
  print(data);

  // destroy all of the threads
  for(int t = 0; t < info->threads; t++) {
    pthread_join(threads[t], NULL);
  }  

  timing.end = clock();

  // print if verbose
  if(info->verbosity == 2) {
    clock_t diff = timing.end - timing.start;
    float millis = diff / CLOCKS_PER_SEC * 1000.0;
    int sec = (int) millis / 1000;
    millis = millis - sec * 1000;

    printf("Time: %d sec %.1f millisec\n", sec, millis);
  }

}

void free_info(info_t* info)
{
  // going to also free global variables here
  pthread_barrier_destroy(&bprint);
  pthread_barrier_destroy(&bwork);
  sem_destroy(&mutex);
  free(info);
}

void free_data(data_t* data)
{
  // stored in col form
  for(int col = 0; col < data->cols; col++) {
    free(data->mat[col]);
  }
  free(data->mat);
  free(data);
}
void free_tinfo(tinfo_t* tinfo)
{
  // do not need to free data_t*
  free(tinfo);
}


