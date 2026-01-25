#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

int in_circle(double x, double y) {
    if (y*y <= 1 - x*x) return 1;

    return 0;
}


double approx_pi(int n) {
    int count = 0;
    double f_x;

    for (int i = 0; i < n; i++) {
        double x = (double) rand() / (double) RAND_MAX;
        double y = (double) rand() / (double) RAND_MAX;

        count += in_circle(x,y);
    }

    return 4.0* (double) count / (double) n;
}

int main(int argc, char* argv[]) {
    /* 
    input:
        num_pts (int)
    */
    if (argc != 2){
        printf("useage: %s <num_pts>\n", argv[0]);
    }
    int n = atoi(argv[1]);

    srand(time(NULL));

    double pi = approx_pi(n);
    
    printf("pi: %f\n", pi);

    return 0;
}

