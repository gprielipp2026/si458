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

void mandelSet(uint8_t* array, uint32_t w, uint32_t h, double xmax, double xmin, double ymax, double ymin, uint32_t processors)
{
    double dx = (xmax - xmin) / w;
    double dy = (ymax - ymin) / h;

    # pragma omp parallel num_threads(processors)
    {
        uint32_t size = omp_get_num_threads();
        uint32_t rank = omp_get_thread_num();

        uint32_t chunksize = h / size;

        uint32_t ystart = chunksize * rank;
        uint32_t yend = chunksize * (rank + 1);

        // give extra work to last thread
        if(rank == size - 1) 
        {
            yend = h;
        }

        for(uint32_t y = ystart; y < yend; y++) 
        {
            for(uint32_t x = 0; x < w; x++) 
            {
                array[x + y*h] = inMandel(xmin + x*dx, ymin + y*dy);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    // should hhave a test here
    uint32_t w=1000, h=850, p=1;
    double xmax=1.0, xmin=-2.0, ymax=1.0, ymin=-1.0;
    uint8_t* array = calloc(w*h, sizeof(uint8_t));
    
    mandelSet(array, w, h, xmax, xmin, ymax, ymin, p);

    free(array);

    return 0;
}