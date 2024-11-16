#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define NUM_THREADS 10


#define NUM_PARTICLES 400000
#define SPEED 0.0004f

#define WIDTH 1600
#define HEIGHT 900

#define BIN_SIZE 0.04
#define GRID_WIDTH 500
#define GRID_HEIGHT 500
// #define MAX_PARTICLES_PER_BIN 256

typedef struct {
    float position[2];
    float velocity[2];
} Particle;

typedef struct {
    uint32_t offset;
    uint16_t total_count;
    uint16_t cur_count;
} Bin;

typedef struct {
    int start_bx;
    int end_bx;
    int start_by;
    int end_by;
    Bin *bins;
    Particle *particles;
} ThreadData;

float random_float() {
    return (float)rand() / RAND_MAX * 2.0f - 1.0f;
}

void update_particles(Particle* particles, int start_a, int end_a, int start_b, int end_b) {

    for (int i = start_a; i < end_a; i++) {
        for (int j = start_b; j < end_b; j++) {
            float dx = particles[i].position[0] - particles[j].position[0];
            float dy = particles[i].position[1] - particles[j].position[1];
            float dsq = dx * dx + dy * dy;
            float d = sqrt(dsq);
            //float dm = d * d * d + 0.01;
            float dm = exp(250.0 * d);
            float fx = dx / dm;
            float fy = dy / dm;
            float mag = 5.5;

            particles[i].velocity[0] += fx * mag;
            particles[i].velocity[1] += fy * mag;

            particles[j].velocity[0] -= fx * mag;
            particles[j].velocity[1] -= fy * mag;
        }
    }
}

void update_particles_self(Particle* particles, int start, int end) {

    for (int i = start; i < end - 1; i++) {
        for (int j = i + 1; j < end; j++) {
            float dx = particles[i].position[0] - particles[j].position[0];
            float dy = particles[i].position[1] - particles[j].position[1];
            float dsq = dx * dx + dy * dy;
            float d = sqrt(dsq);
            //float dm = d * d * d + 0.01;
            float dm = exp(250.0 * d);
            float fx = dx / dm;
            float fy = dy / dm;
            float mag = 5.5;

            particles[i].velocity[0] += fx * mag;
            particles[i].velocity[1] += fy * mag;

            particles[j].velocity[0] -= fx * mag;
            particles[j].velocity[1] -= fy * mag;
        }
    }
}

void update_particles_elementwise(Particle *particles) {
    // Update particle positions based on velocity
    for (int i = 0; i < NUM_PARTICLES; i++) {
        
        float center_mag = 0.00002;
        particles[i].velocity[0] -= center_mag * particles[i].position[0];
        particles[i].velocity[1] -= center_mag * particles[i].position[1];

        particles[i].velocity[0] *= 0.996;
        particles[i].velocity[1] *= 0.996;

        particles[i].position[0] += particles[i].velocity[0];
        particles[i].position[1] += particles[i].velocity[1];
        /*
        if (particles[i].position[0] > 1.0f || particles[i].position[0] < -1.0f)
            particles[i].velocity[0] *= -1;
        if (particles[i].position[1] > 1.0f || particles[i].position[1] < -1.0f)
            particles[i].velocity[1] *= -1;
            */
    }
}

void print_bin(int idx, Bin bin) {
    printf("self interact: bin idx: %d bin offset: %d bin total count: %d\n", 
        idx, bin.offset, bin.total_count
    );
}

// Thread function for parallel execution
void *update_particles_binned_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    Bin *bins = data->bins;
    Particle *particles = data->particles;

    for (int by = data->start_by; by < data->end_by; by++) {
        for (int bx = data->start_bx; bx < data->end_bx; bx++) {
            Bin bin_a = bins[bx + by * GRID_WIDTH];
            int pairs[10] = {-1, -1, 0, -1, 1, -1, -1, 0, 0, 0};
            for (int i = 0; i < 5; i++) {
                int xoff = pairs[i * 2 + 0];
                int yoff = pairs[i * 2 + 1];
                int other_x = bx + xoff;
                int other_y = by + yoff;
                if (other_x > 0 && other_x < GRID_WIDTH && other_y > 0 && other_y < GRID_HEIGHT) {
                    if (xoff == 0 && yoff == 0) {
                        // Self update
                        update_particles_self(particles, bin_a.offset, bin_a.offset + bin_a.total_count);
                    } else {
                        Bin bin_b = bins[bx + xoff + (by + yoff) * GRID_WIDTH];
                        update_particles(
                            particles, 
                            bin_a.offset, bin_a.offset + bin_a.total_count,
                            bin_b.offset, bin_b.offset + bin_b.total_count
                        );
                    }
                }
            }
        }
    }

    return NULL;
}

// Main function to manage pthreads
void update_particles_binned(Bin *bins, Particle *particles) {
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    int chunk_by = GRID_HEIGHT / NUM_THREADS;
    //int chunk_bx = GRID_WIDTH;// / NUM_THREADS;

    // Create threads and assign work
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].start_bx = 0; //(i % NUM_THREADS) * chunk_bx;
        thread_data[i].end_bx = GRID_WIDTH; // (i == NUM_THREADS - 1) ? GRID_WIDTH : (i + 1) * chunk_bx;
        thread_data[i].start_by = chunk_by * i; //(i % NUM_THREADS) * chunk_by;
        thread_data[i].end_by = chunk_by * (i + 1); //(i == NUM_THREADS - 1) ? GRID_HEIGHT : (i + 1) * chunk_by;
        thread_data[i].bins = bins;
        thread_data[i].particles = particles;

        pthread_create(&threads[i], NULL, update_particles_binned_thread, (void *)&thread_data[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* 
void update_particles_binned(Bin *bins, Particle *particles) {
    #pragma omp parallel for
    for (int by = 0; by < GRID_HEIGHT; by++) {
        for (int bx = 0; bx < GRID_WIDTH; bx++) {
            Bin bin_a = bins[bx + by * GRID_WIDTH];
            int pairs[10] = {-1, -1, 0, -1, 1, -1, -1, 0, 0, 0};
            for (int i = 0; i < 5; i++) {
                int xoff = pairs[i * 2 + 0];
                int yoff = pairs[i * 2 + 1];
                int other_x = bx + xoff;
                int other_y = by + yoff;
                if (other_x > 0 && other_x < GRID_WIDTH && other_y > 0 && other_y < GRID_HEIGHT) {
                    if (xoff == 0 && yoff == 0) {
                        //print_bin(bx + by * GRID_WIDTH, bin_a);
                        update_particles_self(particles, bin_a.offset, bin_a.offset + bin_a.total_count);
                    } else {
                        Bin bin_b = bins[bx + xoff + (by + yoff) * GRID_WIDTH];
                        update_particles(
                            particles, 
                            bin_a.offset, bin_a.offset + bin_a.total_count,
                            bin_b.offset, bin_b.offset + bin_b.total_count
                        );
                    }
                }
            }
        }
    }
}
*/

uint32_t position_to_bin_idx(float x, float y) {
    return ((uint32_t) (x / BIN_SIZE + 0.5 * GRID_WIDTH)) + 
           ((uint32_t) (y / BIN_SIZE + 0.5 * GRID_HEIGHT)) * GRID_WIDTH;
}

void clear_bins(Bin *bins) {
    for (int i = 0; i < GRID_HEIGHT * GRID_HEIGHT; i++) {
        bins[i] = (Bin) { 0, 0, 0 };
    }
}

void update_bins(Bin *bins, Particle *particles) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        uint32_t bin_idx = position_to_bin_idx(particles[i].position[0], particles[i].position[1]);
        //if (bin_idx != 0) {
        //    printf("bin idx: %d \n", bin_idx);
        //}
        bins[bin_idx].total_count += 1;
    }
    for (int i = 1; i < GRID_HEIGHT * GRID_WIDTH; i++) {
        bins[i].offset = bins[i-1].total_count + bins[i-1].offset;
    }
}

void sort_into_bins(Bin *bins, Particle *particle_src, Particle *particle_dst) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        uint32_t bin_idx = position_to_bin_idx(particle_src[i].position[0], particle_src[i].position[1]);
        uint32_t dst_idx = bins[bin_idx].offset + bins[bin_idx].cur_count;
        //if (dst_idx != i) {
        particle_dst[dst_idx] = particle_src[i];
        //}
        bins[bin_idx].cur_count += 1;
    }
}

double calculate_elapsed_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Particle Renderer with Motion", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to open GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    printf("initializing with %d particles\n", NUM_PARTICLES);

    Particle *particles = malloc(NUM_PARTICLES * sizeof(Particle));
    Particle *back_particles = malloc(NUM_PARTICLES * sizeof(Particle));

    Bin *bins = malloc(GRID_WIDTH * GRID_HEIGHT * sizeof(Bin));

    float size_sq = sqrt((float) NUM_PARTICLES);
    int size_sq_i = (int) size_sq;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].position[0] = 10.0 * (((float) (i % size_sq_i)) / size_sq - 0.5);//random_float();
        particles[i].position[1] = 10.0 * (((float) i) / (size_sq * size_sq) - 0.5);//random_float();
        particles[i].velocity[0] = random_float() * SPEED;
        particles[i].velocity[1] = random_float() * SPEED;
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Load and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char *vertexSource = "#version 330 core\nlayout(location = 0) in vec2 position;\nvoid main() { gl_PointSize = 3.0; gl_Position = vec4(position, 0.0, 1.0); }";
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragmentSource = "#version 330 core\nout vec4 fragColor;\nvoid main() { fragColor = vec4(1.0, 1.0, 1.0, 1.0); }";
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgram);

    glEnable(GL_PROGRAM_POINT_SIZE); 

    float *particle_pos_data = malloc(NUM_PARTICLES * sizeof(float) * 2);
    
    struct timespec start, end;
    double clear_bins_time = 0, update_bins_time = 0, swap_time = 0, sort_bins_time = 0;
    double update_binned_time = 0, update_elementwise_time = 0, render_time = 0;
    int frame_count = 0;
    double sim_start_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Clear bins timing
        clock_gettime(CLOCK_MONOTONIC, &end);
        clear_bins(bins);
        clear_bins_time += calculate_elapsed_time(start, end);

        // Update bins timing
        clock_gettime(CLOCK_MONOTONIC, &start);
        update_bins(bins, particles);
        clock_gettime(CLOCK_MONOTONIC, &end);
        update_bins_time += calculate_elapsed_time(start, end);

        // Swap particles timing
        clock_gettime(CLOCK_MONOTONIC, &start);
        Particle *temp = particles;
        particles = back_particles;
        back_particles = temp;
        clock_gettime(CLOCK_MONOTONIC, &end);
        swap_time += calculate_elapsed_time(start, end);

        // Sort into bins timing
        clock_gettime(CLOCK_MONOTONIC, &start);
        sort_into_bins(bins, back_particles, particles);
        clock_gettime(CLOCK_MONOTONIC, &end);
        sort_bins_time += calculate_elapsed_time(start, end);

        // Update particles (binned) timing
        clock_gettime(CLOCK_MONOTONIC, &start);
        update_particles_binned(bins, particles);
        clock_gettime(CLOCK_MONOTONIC, &end);
        update_binned_time += calculate_elapsed_time(start, end);

        // Update particles (element-wise) timing
        clock_gettime(CLOCK_MONOTONIC, &start);
        update_particles_elementwise(particles);
        clock_gettime(CLOCK_MONOTONIC, &end);
        update_elementwise_time += calculate_elapsed_time(start, end);

        // Render timing
        clock_gettime(CLOCK_MONOTONIC, &start);

        float ratio = (float) WIDTH / (float) HEIGHT;
        for (int i = 0; i < NUM_PARTICLES; i++) {
            particle_pos_data[i * 2 + 0] = 0.2 * particles[i].position[0];
            particle_pos_data[i * 2 + 1] = 0.2 * particles[i].position[1] * ratio;
        }

        glClear(GL_COLOR_BUFFER_BIT);


        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 2 * sizeof(float), particle_pos_data, GL_DYNAMIC_DRAW);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        glfwSwapBuffers(window);
        glfwPollEvents();

        clock_gettime(CLOCK_MONOTONIC, &end);
        render_time += calculate_elapsed_time(start, end);

        frame_count++;
        if (glfwGetTime() - sim_start_time >= 1.0) {
            printf("Average times per stage (seconds):\n");
            printf("Clear Bins: %f\n", clear_bins_time / frame_count);
            printf("Update Bins: %f\n", update_bins_time / frame_count);
            printf("Swap Particles: %f\n", swap_time / frame_count);
            printf("Sort Into Bins: %f\n", sort_bins_time / frame_count);
            printf("Update Particles (Binned): %f\n", update_binned_time / frame_count);
            printf("Update Particles (Element-wise): %f\n", update_elementwise_time / frame_count);
            printf("total sim time: %f\n", (clear_bins_time + swap_time + 
                sort_bins_time + update_binned_time + update_elementwise_time) / frame_count);
            printf("Render: %f\n", render_time / frame_count);
            printf("-----------------------------\n");

            // Reset counters
            clear_bins_time = update_bins_time = swap_time = sort_bins_time = update_binned_time = update_elementwise_time = render_time = 0;
            frame_count = 0;
            sim_start_time = glfwGetTime();
        }
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    free(particles);
    free(bins);
    free(back_particles);
    glfwTerminate();

    return 0;
}

