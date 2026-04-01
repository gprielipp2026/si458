#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <omp.h>

uint8_t inMandel(double x, double y)
{
    double complex c = x + y*I;
    double complex Zn = 0;
    
    for(uint8_t i = 0; i < 100; i++)
    {
        Zn = Zn * Zn + c;
    }

    double real = creal(Zn);
    double imag = cimag(Zn);

    // a^2 + b^2 = c^2
    double mag = real * real + imag * imag;

    return mag < 4.0;
}

void mandelSet(uint8_t* array, const uint32_t w, const uint32_t h, double xmax, double xmin, double ymax, double ymin, uint32_t processors)
{
    double dx = (xmax - xmin) / (double)w;
    double dy = (ymax - ymin) / (double)h;

    # pragma omp parallel num_threads(processors) shared(w) shared(h) shared(dx) shared(dy) shared(xmin) shared(ymin)
    {
        uint32_t size = omp_get_num_threads();
        uint32_t rank = omp_get_thread_num();

        uint32_t chunksize = h / size;

        uint32_t ystart = chunksize * rank;
        uint32_t yend = chunksize * (rank + 1);
        
        // give extra work to last thread
        if(rank == (size - 1))
        {
            yend = h;
        }
       
        // debug 
        // printf("%2d/%2d: %dx%d   %4d - %4d    (%4d)\n", rank, size, w, h, ystart, yend, yend - ystart);

        for(uint32_t y = ystart; y < yend; y++) 
        {
            for(uint32_t x = 0; x < w; x++) 
            {
                *(array + x + y*h) = inMandel(xmin + x*dx, ymin + y*dy);
            }
        }
    }
}

void print(uint8_t *array, uint32_t w, uint32_t h)
{
  for(uint32_t y = 0; y < h; y++)
  {
    for(uint32_t x = 0; x < w; x++)
    {
      printf("%d", array[x + y * h]);// ? 0xf0c8:0x25a1);
    }
    printf("\n");
  }
}

int main(int argc, char* argv[])
{
  if(argc != 4) {
    printf("usage: %s <width> <height> <processors>\n", argv[0]);
    exit(0);
  }

    // should hhave a test here
    uint32_t w=atoi(argv[1]), h=atoi(argv[2]), p=atoi(argv[3]);
    double xmax=1.0, xmin=-2.0, ymax=1.0, ymin=-1.0;
    uint8_t* array = calloc(w*h, sizeof(uint8_t));
    
    mandelSet(array, w, h, xmax, xmin, ymax, ymin, p);

    /*print(array, w, h);*/

    free(array);

    return 0;
}
