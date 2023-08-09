#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <SDL2/SDL.h>
#include <signal.h>

// Screen dimensions
#define SCREEN_WIDTH 272
#define SCREEN_HEIGHT 480

// Buffer size and block dimensions
#define BUFFER_SIZE SCREEN_WIDTH * SCREEN_HEIGHT * 3
#define BLOCK_WIDTH 20
#define BLOCK_HEIGHT 20

pthread_t read_thread, plot_thread;

/**
 * @brief Circular buffer structure to hold pixel values.
 */
typedef struct {
    int* buffer;
    size_t size;
    size_t start;
    size_t end;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

CircularBuffer cbuffer;

/**
 * @brief Initializes the circular buffer.
 * 
 * @param size Size of the buffer.
 */
void init_buffer(size_t size) {
    cbuffer.buffer = (int*)malloc(size * sizeof(int));
    cbuffer.size = size;
    cbuffer.start = 0;
    cbuffer.end = 0;
    pthread_mutex_init(&cbuffer.mutex, NULL);
    pthread_cond_init(&cbuffer.not_empty, NULL);
    pthread_cond_init(&cbuffer.not_full, NULL);
}

/**
 * @brief Pushes a value into the circular buffer.
 * 
 * @param value The value to push.
 */
void push_buffer(int value) {
    pthread_mutex_lock(&cbuffer.mutex);
    while ((cbuffer.end + 1) % cbuffer.size == cbuffer.start) {
        pthread_cond_wait(&cbuffer.not_full, &cbuffer.mutex);
    }
    cbuffer.buffer[cbuffer.end] = value;
    cbuffer.end = (cbuffer.end + 1) % cbuffer.size;
    pthread_cond_signal(&cbuffer.not_empty);
    pthread_mutex_unlock(&cbuffer.mutex);
}

/**
 * @brief Pops a value from the circular buffer.
 * 
 * @return The popped value.
 */
int pop_buffer() {
    pthread_mutex_lock(&cbuffer.mutex);
    while (cbuffer.start == cbuffer.end) {
        pthread_cond_wait(&cbuffer.not_empty, &cbuffer.mutex);
    }
    int value = cbuffer.buffer[cbuffer.start];
    cbuffer.start = (cbuffer.start + 1) % cbuffer.size;
    pthread_cond_signal(&cbuffer.not_full);
    pthread_mutex_unlock(&cbuffer.mutex);
    return value;
}

// Global variables and mutex for restarting read thread
volatile int read_restart = 0;
pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Read thread function to read random values and push them to the buffer.
 * 
 * @param arg Not used (can be NULL).
 * @return NULL.
 */
void* read_file_thread() {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("Error opening /dev/urandom");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&read_mutex);
        if (read_restart) {
            lseek(fd, 0, SEEK_SET); // Move the file pointer back to the beginning
            read_restart = 0;
        }
        pthread_mutex_unlock(&read_mutex);

        int random_value;
        if (read(fd, &random_value, sizeof(int)) == sizeof(int)) {
            push_buffer(random_value);
        }
    }

    close(fd);
    return NULL;
}

/**
 * @brief Plot thread function to render the buffer contents on the screen.
 * 
 * @param arg Not used (can be NULL).
 * @return NULL.
 */
void* plot_thread_func() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Circular Buffer Plot",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        exit(EXIT_FAILURE);
    }

    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto cleanup;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        pthread_mutex_lock(&cbuffer.mutex);  // Lock the buffer for reading

        int blockWidth = BLOCK_WIDTH;
        int blockHeight = BLOCK_HEIGHT;

        for (size_t i = cbuffer.start; i != cbuffer.end; i = (i + 1) % cbuffer.size) {
            int value = cbuffer.buffer[i];

            // Calculate block position (10x10 pixel block)
            int xPos = (int)(i % (SCREEN_WIDTH / blockWidth)) * blockWidth;
            int yPos = (int)(i / (SCREEN_WIDTH / blockWidth)) * blockHeight;

            // Determine the color based on the random value
            Uint8 red = (Uint8)(value % 256);
            Uint8 green = (Uint8)((value / 2) % 256);
            Uint8 blue = (Uint8)((value / 4) % 256);

            SDL_SetRenderDrawColor(renderer, red, green, blue, 255);

            // Draw the block
            SDL_Rect blockRect = {xPos, yPos, blockWidth, blockHeight};
            SDL_RenderFillRect(renderer, &blockRect);
        }

        pthread_mutex_unlock(&cbuffer.mutex);  // Unlock the buffer after reading

        SDL_RenderPresent(renderer);
        SDL_Delay(1);  // Delay to control rendering speed
    }

cleanup:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    raise(SIGTERM);
    return NULL;
}

/**
 * @brief SIGINT and SIGTERM handler.
 * 
 * @return Exit status.
 */
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        
        // Cancel the threads
        pthread_cancel(read_thread);
        pthread_cancel(plot_thread);

        free(cbuffer.buffer);
        
        // Exit the program
        exit(EXIT_SUCCESS);
    }
}

/**
 * @brief Main function.
 * 
 * @return Exit status.
 */
int main() {
    init_buffer(BUFFER_SIZE);

    // Create read and plot threads
    pthread_create(&read_thread, NULL, read_file_thread, NULL);
    pthread_create(&plot_thread, NULL, plot_thread_func, NULL);

    // Install the signal handler
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Wait for the threads to finish before exiting
    pthread_join(read_thread, NULL);
    pthread_join(plot_thread, NULL);

    // Clean up resources
    free(cbuffer.buffer);
    pthread_mutex_destroy(&cbuffer.mutex);
    pthread_cond_destroy(&cbuffer.not_empty);
    pthread_cond_destroy(&cbuffer.not_full);

    return 0;
}
