#include <stdio.h>
#include <stdlib.h>
#include <math.h>

__global__ void gpu_ln(float x, float *series, int N)
{
  int k = threadIdx.x + blockIdx.x * blockDim.x;
  
  // trying the "hyperbolic arctangent" series

  // float term = (j % 2 == 0 ? -1:1) * powf(x, (float)j) / j; 
  float t = (x - 1) / (x + 1);
  float term = powf(t, 2 * k + 1) / (2 * k + 1);

  *(series + k) = term;

  // do some tricky stuff to add up all values (some sort of tree maybe)
  //__syncthreads();
  
  // for now to test:
  if(k == 0)
  {
    float sum = 0.0f;
    for(int i = 0; i < N; i++)
    {
      sum += series[i];
    }
    sum *= 2;
   // series[0] = sum;
  }
}

void cpu_ln(float x, float *series, int N)
{
  float t = (x - 1.0f) / (x + 1.0f);
  for(int k = 0; k < N; k++)
  {
    series[k] = powf(t, 2 * k + 1.0f) / (2 * k + 1.0f);
  }
}

int main(int argc, char* argv[])
{
  if(argc != 3) 
  {
    fprintf(stderr, "usage: %s <# of terms> <value>\n", argv[0]);
    exit(0);
  }

  int cores = atoi(argv[1]);
  float x = strtof(argv[2], NULL);

  if(x <= 0.0f) {
    printf("Error: ln(%f) = undefined\n", x);
    exit(0);
  }
  //else if(x < 1 || x > 2) {
  //  printf("Error: Maclurian Series of %f is wrong\n", x);
  //  exit(0);
  //}

  float *series = (float*)calloc(sizeof(float), cores);
  float *dseries;
 
  cudaMalloc( (void**)&dseries, sizeof(float)*cores);

  cudaMemcpy(dseries, series, sizeof(float)*cores, cudaMemcpyHostToDevice);

  int threadsPerBlock = 128; // have 64 FPUs, 2 threads per should be okay
  int blocks = (cores + threadsPerBlock - 1) / threadsPerBlock;
  gpu_ln<<< blocks, threadsPerBlock >>>(x, dseries, cores);

  cudaMemcpy(series, dseries, sizeof(float)*cores, cudaMemcpyDeviceToHost);

  printf("ln(%f) = %f\n", x, series[0]);
  
  //cpu_ln(x, series, cores);

  /* */
  float sum = 0.0f;
  for(int i = 0; i < cores; i++)
  {
    sum += series[i];
    printf("%d: %f\t\t\tsum: %f\n", i, series[i], sum);
  }
  sum *= 2;
  printf("ln(%f) = %f\n", x, sum);
  /**/

  return 0;
}
