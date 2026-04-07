#include <stdio.h>
#include <stdlib.h>

// tells nvcc that this is a gpu function
__global__ void vector_add(float *c, float* a, float* b, int N) 
{
  // gpu function  
  int i = threadIdx.x;
  c[i] = a[i] + b[i];
}



int main()
{
  int N = 1024;

  // host pointers
  float *a, *b, *c;

  // device pointers
  float *da, *db, *dc;

  // allocate host mem
  a = (float*)malloc(sizeof(*a) * N);
  b = (float*)malloc(sizeof(*b) * N);
  c = (float*)malloc(sizeof(*c) * N);
 
  // allocate device mem
  cudaMalloc( (void**)&da, sizeof(float)*N );
  cudaMalloc( (void**)&db, sizeof(float)*N );
  cudaMalloc( (void**)&dc, sizeof(float)*N );

  // initialize host array
  for (int i = 0; i < N; i++)
  {
    a[i] = 1.0f;
    b[i] = 2.0f;
  }

  // copy mem to GPU
  cudaMemcpy(da, a, sizeof(float)*N, cudaMemcpyHostToDevice);
  cudaMemcpy(db, b, sizeof(float)*N, cudaMemcpyHostToDevice);

  // run the vector add
  // <<< blocks, threads>>>
  vector_add<<<1, N>>>(dc, da, db, N); 
 
  cudaMemcpy(c, dc, sizeof(float)*N, cudaMemcpyDeviceToHost);

  for (int i = 0; i < N; i++)
    printf("%.1f ", c[i]);
  printf("\n");

  free(a);
  free(b);
  free(c);

  // free device data
  cudaFree(da);
  cudaFree(db);
  cudaFree(dc);

  return 0;
}
