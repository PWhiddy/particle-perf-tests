#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define NUM_PARTICLES 1000000
#define BIN_SIZE 10
#define GRID_WIDTH 100
#define GRID_HEIGHT 100
#define MAX_PARTICLES_PER_BIN 256

typedef struct {
    double x;
    double y;
} Particle;

typedef struct {
    Particle particles[MAX_PARTICLES_PER_BIN];
    int count;
} Bin;

// Function to get the bin index for a given particle
inline int get_bin_index(double x, double y) {
    int x_index = (int)(x / BIN_SIZE);
    int y_index = (int)(y / BIN_SIZE);
    return y_index * GRID_WIDTH + x_index;
}

// Main function to sort particles into bins in parallel
int main() {
    Particle particles[NUM_PARTICLES];
    Bin bins[GRID_WIDTH * GRID_HEIGHT];

    // Initialize bins
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        bins[i].count = 0;
    }

    // Timing the particle generation
    clock_t start_gen = clock();
    // Generate some random particles
    srand(time(NULL));
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = ((double)rand() / RAND_MAX) * 1000.0;
        particles[i].y = ((double)rand() / RAND_MAX) * 1000.0;
    }
    clock_t end_gen = clock();
    double duration_gen = (double)(end_gen - start_gen) / CLOCKS_PER_SEC;
    printf("Time taken to generate particles: %f seconds\n", duration_gen);

    // Timing the binning process
    clock_t start_bin = clock();
    
    #pragma omp parallel for
    for (int i = 0; i < NUM_PARTICLES; i++) {
        Particle p = particles[i];
        int bin_index = get_bin_index(p.x, p.y);
        
        #pragma omp critical
        {
            if (bins[bin_index].count < MAX_PARTICLES_PER_BIN) {
                bins[bin_index].particles[bins[bin_index].count++] = p;
            }
        }
    }
    clock_t end_bin = clock();
    double duration_bin = (double)(end_bin - start_bin) / CLOCKS_PER_SEC;
    printf("Time taken to bin particles: %f seconds\n", duration_bin);
/*
    // Timing the bin info printing
    clock_t start_print = clock();
    // Print some information about the bins
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        if (bins[i].count > 0) {
            int x = i % GRID_WIDTH;
            int y = i / GRID_WIDTH;
            printf("Bin (%d, %d) has %d particles\n", x, y, bins[i].count);
        }
    }
*/
    clock_t end_print = clock();
 //   double duration_print = (double)(end_print - start_print) / CLOCKS_PER_SEC;
    //printf("Time taken to print bin information: %f seconds\n", duration_print);

    // Total time
    double total_duration = (double)(end_print - start_gen) / CLOCKS_PER_SEC;
    printf("Total time taken: %f seconds\n", total_duration);

    return 0;
}

