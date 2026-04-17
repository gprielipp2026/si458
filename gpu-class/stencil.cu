#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/*
  stencil (1D):
    have to sum up all neighbors in radius of a "cell" in an array

  stencils are used all the time for differential equation based simulations;
    ex:
      - nuclear chains
      - air flow
      - growth patterns
      - the atmosphere



 */

#define N 36
// warp is 32 threads
// each thread is dealing with 2*radius
// so maximizing warp, N is warp + 2*radius (32 + 4 in this case)
#define RADIUS 2
#define BLOCKS 1

__global__ void stencil_1d(int* in, int* out) 
{
  // declare array into shared memory (data shared between threads)
  // note: not shared across blocks
  __shared__ int temp[BLOCKS + 2*RADIUS];

  // global index across all blocks
  int gidx = blockIdx.x * blockDim.x + threadIdx.x; 
  /*
    there is a .y and a .z;
    cuda compiles with context clues, it treats everything in the dimensions
    that you use. IE: .x => 1D, .x + .y => 2D, .x + .y + .z => 3D

    depends on the problem that you are trying to solve; it compiles the same
    and is handled by the compiler creators. It is only there to allow us to
    better think through our problems (eg: MRI image parsing => 3D problem; each
    layer can be handled by a different .z layer and then the threads can be
    split across the .x and .y of the image).
   */ 
  // local index within block
  int lidx = threadIdx.x + RADIUS;

  // each thread moves its piece of data to shared memory
  temp[lidx] = in[gidx + RADIUS];
  // the first thread move RADIUS pieces of data to shared
  if(threadIdx.x == 0) 
  {
    for(int i = 0; i < RADIUS; i++) 
      temp[lidx- RADIUS + i] = in[gidx + i];
    
  }
  // the last thread move RADIUS pieces of data to shared
  if(threadIdx.x == blockDim.x-1) 
  {
    for(int i = 0; i < RADIUS; i++) 
      temp[lidx + RADIUS - i] = in[gidx - i];
  }

  // sync the threads to avoid race conditions
  __syncthreads();

  // do the computation
  int sum = 0; 
  for(int i = -RADIUS; i <= RADIUS; i++)
  {
    sum += temp[lidx + i]; 
  }

  out[gidx] = sum;
}

int main(int argc, char* argv[])
{
  // host pointers
  int *ha, *hb;
  // device pointers
  int *da, *db;
 
  // allocate host memory
  ha = (int*)malloc(sizeof(int)*N);
  hb = (int*)malloc(sizeof(int)*(N - 2*RADIUS));

  // allocate device memory
  cudaMalloc((void**)&da, sizeof(int)*N);
  cudaMalloc((void**)&db, sizeof(int)*(N - 2*RADIUS));

  // initialize host array
  for(int i = 0; i < N; i++) 
  {
    ha[i] = i;
  }

  // copy memory to device
  cudaMemcpy(da, ha, sizeof(int)*N, cudaMemcpyHostToDevice);
  
  // call GPU functions
  stencil_1d<<< BLOCKS, N - 2*RADIUS >>>(da, db);

  // copy memory from device
  cudaMemcpy(hb, db, sizeof(int)*(N - 2*RADIUS), cudaMemcpyDeviceToHost);

  // display results
  for(int i = 0; i < N-2*RADIUS; i++)
  {
    printf("%d ", hb[i]);
  }
  printf("\n");

  return 0;
}
