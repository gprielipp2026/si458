// gets rid of pthread_barrier_t not defined
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <semaphore.h>

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

pthread_barrier_t bprint, bwork;

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

    free_gol(gol);

    time_diff(&tinfo);
    time_print(&tinfo);

    return 0;
}

// ---------- func defs ------------
// private
typedef struct {
    int X, Y;
} pos_t;
inline pos_t ind(int x, int y) 
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

void file_mat(gol_t* gol) 
{
    FILE* file = fopen(gol->args->filename, "r");

    int N;
    fscanf(file, "%d %d %d", &gol->args->cols, &gol->args->rows, &N);

    int x, y;
    while(N--) 
    {
        fscanf(file, "%d %d", &x, &y);
        pos_t pos = ind(x, y);
        gol->write[pos.X][pos.Y] = 1;
    }

    fclose(file);
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
        pthread_barrier_wait(&bprint);

        // swap the mat's
        swap(gol);

        // update the halo
        update_halo(gol);

        // print if need be
        if(gol->args->freq > 0 && i % gol->args->freq == 0) 
        {
            for(int row = 0; row < gol->args->rows; row++) 
            {
                for(int col = 0; col < gol->args->cols; col++) 
                {
                    pos_t pos = ind(col, row);
                    printf("%s ", gol->read[pos.X][pos.Y] ? "\u2b1c" : "\u2b1b");
                }
            }
        }

        pthread_barrier_wait(&bwork);
    }
}
// pthread
void worker(void* arg) 
{
    thread_t* thread = (thread_t*)arg;
    gol_t* gol = (gol_t*) thread->gol;
    for(int i = 0; i < gol->args->gens; i++) {
        pthread_barrier_wait(&bprint);
        pthread_barrier_wait(&bwork);
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
    
}

void time_print(tinfo_t* tinfo)
{

}

args_t* parse_args(int argc, char* argv[])
{

}

gol_t* init_gol(args_t* args)
{

}

void simulate(gol_t* gol)
{

}

void free_gol(gol_t* gol)
{

}

