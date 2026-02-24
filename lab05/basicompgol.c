// gets rid of pthread_barrier_t not defined
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h> 
#include <sys/time.h> 
#include <string.h>
#include <semaphore.h>
#include <omp.h>

// -------------- data ------------

typedef struct {
    int gens, freq, threads, verb;
    int rows, cols;
    bool useFn;
    union {
        int seed;
        char* filename;
    };
} args_t;

typedef struct {
    args_t* args;
    int **read, **write;
} gol_t;

typedef struct {
    int start, end, id;
    gol_t* gol;
} thread_t;

typedef struct {
    clock_t cstart, cend;
    struct timeval wstart, wend;
    struct {
        int s, us;
    } cdiff, wdiff;
} tinfo_t;

// pthread_barrier_t bprint, bwork;

// ------------ funcs -------------

void time_start(tinfo_t* tinfo);
void time_stop(tinfo_t* tinfo);
void time_diff(tinfo_t* tinfo);
void time_print(tinfo_t* tinfo);

args_t* parse_args(int argc, char* argv[]);
gol_t* init_gol(args_t* args);
void simulate(gol_t* gol);
void free_gol(gol_t* gol);

// ------------- main -------------

int main(int argc, char* argv[]) {

    args_t* args = parse_args(argc, argv);
    gol_t* gol = init_gol(args);

    tinfo_t tinfo;
    time_start(&tinfo);
    simulate(gol);
    time_stop(&tinfo);

    time_diff(&tinfo);
    if(args->verb >= 2) time_print(&tinfo);
    
    free_gol(gol);

    return 0;
}

// ---------- func defs ------------
// private
typedef struct {
    int X, Y;
} pos_t;
pos_t ind(int x, int y) 
{
    return (pos_t){.X = x+1, .Y = y+1};
}

int neighbors(pos_t pos, gol_t* gol)
{
    int count = 0;
    for(int xoff = -1; xoff <= 1; xoff++) 
    {
        for(int yoff = -1; yoff <= 1; yoff++)
        {
            if(xoff == 0 && yoff == 0) continue;
            count += gol->read[pos.X + xoff][pos.Y + yoff];
        }
    }
    return count;
}

void init_mat(gol_t* gol) 
{
    gol->read = malloc(sizeof(int*) * (gol->args->cols + 2));
    gol->write = malloc(sizeof(int*) * (gol->args->cols + 2));

    for(int col = 0; col < gol->args->cols + 2; col++) 
    {
        gol->read[col] = calloc(gol->args->rows + 2,  sizeof(int));
        gol->write[col] = calloc(gol->args->rows + 2, sizeof(int));
    }
}

void rand_mat(gol_t* gol) 
{
    init_mat(gol);

    srand(gol->args->seed);
    for(int col = 0; col < gol->args->cols; col++) 
    {
        for(int row = 0; row < gol->args->rows; row++)
        {
            pos_t pos = ind(col, row);
            gol->write[pos.X][pos.Y] = rand() % 2;
        }
    }
}

int file_mat(gol_t* gol) 
{
    FILE* file = fopen(gol->args->filename, "r");
    
    if(!file) return 1;

    int N;
    fscanf(file, "%d %d %d", &gol->args->cols, &gol->args->rows, &N);
    
    init_mat(gol);
    

    int x, y;
    while(N--) 
    {
        fscanf(file, "%d %d", &y, &x);
        pos_t pos = ind(x, y);
        gol->write[pos.X][pos.Y] = 1;
    }

    fclose(file);

    return 0;
}

void swap(gol_t* gol) 
{
    int** tmp = gol->read;
    gol->read = gol->write;
    gol->write = tmp;
}

void update_halo(gol_t* gol)
{
    // edges = opposite edges
    // horizontal
    int top = 0, 
        bot = gol->args->rows + 1; // last ind = rows + 2 - 1; last ind = rows + 1;
    for(int col = 0; col < gol->args->cols; col++) 
    {
        gol->read[col+1][top] = gol->read[col+1][bot-1];
        gol->read[col+1][bot] = gol->read[col+1][top+1];
    }

    // vertical
    int l = 0,
        r = gol->args->cols + 1;
    for(int row = 0; row < gol->args->rows; row++) 
    {
        gol->read[l][row+1] = gol->read[r-1][row+1];
        gol->read[r][row+1] = gol->read[l+1][row+1];
    }

    // corners = opposite corner
    gol->read[l][top] = gol->read[r-1][bot-1];
    gol->read[l][bot] = gol->read[r-1][top+1];
    gol->read[r][top] = gol->read[l+1][bot-1];
    gol->read[r][bot] = gol->read[l+1][top+1];

}

// threads
// main thread
void disp_grid(gol_t* gol) 
{
    for(int i = 0; i < gol->args->gens; i++)
    {
      # pragma omp barrier
        // pthread_barrier_wait(&bprint);

        // swap the mat's
        swap(gol);

        // update the halo
        update_halo(gol);

        // print if need be+1
        if(gol->args->freq > 0 && i % gol->args->freq == 0) 
        {
            printf("\n%d:\n--------------------------------------------------\n", i);
            for(int row = 0; row < gol->args->rows; row++) 
            {
                for(int col = 0; col < gol->args->cols; col++) 
                {
                    pos_t pos = ind(col, row);
                    // for fun
                    // printf("%s", gol->read[pos.X][pos.Y] == 1 ? "\u2b1b" : "\u2b1c");
                    printf("%d ", gol->read[pos.X][pos.Y]);
                }
                printf("\n");
            }
            printf("--------------------------------------------------\n");
        }

      # pragma omp barrier
        // pthread_barrier_wait(&bwork);
    }

    if(gol->args->freq > 0)
    {
        printf("\nfinal:\n--------------------------------------------------\n");
        for(int row = 0; row < gol->args->rows; row++) 
        {
            for(int col = 0; col < gol->args->cols; col++) 
            {
                pos_t pos = ind(col, row);
                // for fun
                // printf("%s", gol->read[pos.X][pos.Y] ? "\u2b1b" : "\u2b1c");
                printf("%d ", gol->read[pos.X][pos.Y]);
            }
            printf("\n");
        }
        printf("--------------------------------------------------\n");
    }
}
// pthread
void worker(thread_t* thread) 
{
    gol_t* gol = (gol_t*) thread->gol;
    for(int i = 0; i < gol->args->gens; i++) {
      # pragma omp barrier
        // pthread_barrier_wait(&bprint);
      # pragma omp barrier
        // pthread_barrier_wait(&bwork);
        for(int col = thread->start; col < thread->end; col++)
        {
            for(int row = 0; row < gol->args->rows; row++) 
            {
                pos_t pos = ind(col, row);
                int count = neighbors(pos, gol);
                int cell = gol->read[pos.X][pos.Y];
                if(cell && (count == 2 || count == 3)) gol->write[pos.X][pos.Y] = 1;
                else if(!cell && count == 3) gol->write[pos.X][pos.Y] = 1;
                else gol->write[pos.X][pos.Y] = 0;
            }
        }
    }
    // pthread_exit(0);
}

// public
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

args_t* parse_args(int argc, char* argv[])
{
    if(argc != 5) 
    {
        printf("usage: %s <generations> <display frequency> <threads> <verbosity>\n", argv[0]);
        exit(1);
    }

    args_t* args = malloc(sizeof(*args));

    args->gens = atoi(argv[1]);
    args->freq = atoi(argv[2]);
    args->threads = atoi(argv[3]);
    if(args->threads < 1) args->threads = 1; // just overwrite that

    args->verb = atoi(argv[4]);

    printf("num rows: ");
    scanf("%d%*c", &args->rows);

    if(args->rows > 0) 
    {
        printf("num cols: ");
        scanf("%d%*c", &args->cols);
        printf("seed: ");
        scanf("%d%*c", &args->seed);
    }
    else 
    {
        args->useFn = true;
        args->seed = time(NULL);
        args->filename = malloc(sizeof(char)*100);
        printf("filename: ");
        fgets(args->filename, sizeof(char)*100, stdin);
        args->filename[strlen(args->filename)-1] = '\0';
    }

    return args;
}

gol_t* init_gol(args_t* args)
{
    gol_t* gol = malloc(sizeof(*gol));

    gol->args = args;

    if(args->useFn) 
    {
        if( file_mat(gol) )
        {
            free_gol(gol);
            printf("Error opening '%s'\n", args->filename);
            exit(1);
        }
    }
    else rand_mat(gol);

    return gol;
}

void simulate(gol_t* gol)
{
    // pthread_barrier_init(&bprint, NULL, gol->args->threads+1);
    // pthread_barrier_init(&bwork, NULL, gol->args->threads+1);

    // pthread_t tids[gol->args->threads];
    thread_t targs[gol->args->threads];

    int perT = gol->args->cols / gol->args->threads, 
    rem = gol->args->cols % gol->args->threads;
    int start = 0, 
        end;
    // create the threads
    for(int t = 0; t < gol->args->threads; t++)
    {
        end = start + perT + (rem-- > 0);
        targs[t] = (thread_t){
            .start = start,
            .end   = end,
            .id    = t,
            .gol   = gol
        };
        
        if(gol->args->verb > 0 && gol->args->verb != 3) 
        {
            printf("tid %d\tcolumns:\t%d:%d\t(%d)\n", t, start, end, end-start);
        }
        
        // pthread_create(&tids[t], NULL, worker, (void*)&targs[t]);
        
        start = end;
    }

    // set main thread to do work
    # pragma omp master
    disp_grid(gol);
    
    # pragma omp parallel num_threads(gol->args->threads) 
    {
      thread_t targ = targs[ omp_get_thread_num() ];
      worker(&targ);
    }

    // join the threads
    // for(int t = 0; t < gol->args->threads; t++)
    // {
    //     pthread_join(tids[t], NULL);
    // }

    // pthread_barrier_destroy(&bprint);
    // pthread_barrier_destroy(&bwork);
}

void free_gol(gol_t* gol)
{
    if(gol->read != NULL)
    {
        for(int col = 0; col < gol->args->cols+2; col++)
        {
            free(gol->read[col]);
            free(gol->write[col]);
        }
        free(gol->read);
        free(gol->write);
    }

    if(gol->args->useFn) free(gol->args->filename);
    free(gol->args);
    free(gol);
}

