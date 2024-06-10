#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "stack.h"

// gcc -DBOARD_SIZE=15 stack.c nqueens.c -lpthread
#ifndef BOARD_SIZE
#define BOARD_SIZE 15
#endif

#define BUFFER_SIZE 4
int find_n_queens_with_prepositions(int N, struct stack_t *prep);

struct buffer_t {
    struct stack_t *results[BUFFER_SIZE];
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

struct buffer_t buffer;
int total_solutions = 0;
int terminate = 0;

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nTotal solutions found: %d\n", total_solutions);
        exit(EXIT_SUCCESS);
    }
}

int row(int cell) {
    return cell / BOARD_SIZE;
}

int col(int cell) {
    return cell % BOARD_SIZE;
}

int is_feasible(struct stack_t *queens) {
    int board[BOARD_SIZE][BOARD_SIZE];
    memset(board, 0, sizeof(board));

    for (int i = 0; i < get_size(queens); i++) {
        int cell;
        get_elem(queens, i, &cell);

        int r = row(cell);
        int c = col(cell);

        if (board[r][c] != 0) {
            return 0;
        }

        for (int y = 0; y < BOARD_SIZE; y++) board[y][c] = 1;
        for (int x = 0; x < BOARD_SIZE; x++) board[r][x] = 1;

        for (int y = r + 1, x = c + 1; y < BOARD_SIZE && x < BOARD_SIZE; y++, x++) board[y][x] = 1;
        for (int y = r + 1, x = c - 1; y < BOARD_SIZE && x >= 0; y++, x--) board[y][x] = 1;
        for (int y = r - 1, x = c + 1; y >= 0 && x < BOARD_SIZE; y--, x++) board[y][x] = 1;
        for (int y = r - 1, x = c - 1; y >= 0 && x >= 0; y--, x--) board[y][x] = 1;
    }

    return 1;
}

void print_placement(struct stack_t *queens) {
    for (int i = 0; i < queens->size; i++) {    
        int queen;
        get_elem(queens, i, &queen);
        printf("[%d,%d] ", row(queen), col(queen));
    }
    printf("\n");
}

void *producer(void *arg) {
    int N = *((int *)arg);
    struct stack_t *queens = create_stack(BOARD_SIZE);
    push(queens, 0);

    while (!is_empty(queens)) {
        int latest_queen;
        top(queens, &latest_queen);

        if (latest_queen == BOARD_SIZE * BOARD_SIZE) {
            pop(queens, &latest_queen);
            if (!is_empty(queens)) {
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                break;
            }
            continue;
        }

        if (is_feasible(queens)) {
            if (get_size(queens) == N) {
                pthread_mutex_lock(&buffer.mutex);
                
                while (buffer.count == BUFFER_SIZE && !terminate) {
                    pthread_cond_wait(&buffer.not_full, &buffer.mutex);
                }

                if (terminate) {
                    pthread_mutex_unlock(&buffer.mutex);
                    break;
                }

                buffer.results[buffer.in] = create_stack(BOARD_SIZE);
                memcpy(buffer.results[buffer.in], queens, sizeof(struct stack_t));
                buffer.in = (buffer.in + 1) % BUFFER_SIZE;
                buffer.count++;

                pthread_cond_signal(&buffer.not_empty);
                pthread_mutex_unlock(&buffer.mutex);

                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                push(queens, latest_queen + 1);
            }
        } else {
            pop(queens, &latest_queen);
            push(queens, latest_queen + 1);
        }
    }
    delete_stack(queens);
    return NULL;
}

void *consumer(void *arg) {
    int N = *((int *)arg);

    while (!terminate) {
        pthread_mutex_lock(&buffer.mutex);

        while (buffer.count == 0 && !terminate) {
            pthread_cond_wait(&buffer.not_empty, &buffer.mutex);
        }

        if (terminate) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        struct stack_t *queens = buffer.results[buffer.out];
        buffer.out = (buffer.out + 1) % BUFFER_SIZE;
        buffer.count--;

        pthread_cond_signal(&buffer.not_full);
        pthread_mutex_unlock(&buffer.mutex);

        find_n_queens_with_prepositions(N, queens);
        delete_stack(queens);
    }
    return NULL;
}

int find_n_queens_with_prepositions(int N, struct stack_t *prep) {
    struct stack_t *queens = create_stack(BOARD_SIZE);
    queens->capacity = prep->capacity;
    queens->size = prep->size;
    memcpy(queens->buffer, prep->buffer, prep->size * sizeof(int));

    while (!is_empty(queens)) {
        int latest_queen;
        top(queens, &latest_queen);

        if (latest_queen == BOARD_SIZE * BOARD_SIZE) {
            pop(queens, &latest_queen);
            if (!is_empty(queens)) {
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                break;
            }
            continue;
        }

        if (is_feasible(queens)) {
            if (get_size(queens) == N) {
                pthread_mutex_lock(&buffer.mutex);
                total_solutions++;
                pthread_mutex_unlock(&buffer.mutex);

                print_placement(queens);

                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                push(queens, latest_queen + 1);
            }
        } else {
            pop(queens, &latest_queen);
            push(queens, latest_queen + 1);
        }
    }
    delete_stack(queens);
    return 0;
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int opt;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-t num_threads]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, handle_signal);

    buffer.count = 0;
    buffer.in = 0;
    buffer.out = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);
    pthread_cond_init(&buffer.not_full, NULL);

    pthread_t producers[num_threads];
    pthread_t consumers[num_threads];
    int N = BOARD_SIZE;

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&producers[i], NULL, producer, &N);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&consumers[i], NULL, consumer, &N);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(producers[i], NULL);
    }

    pthread_mutex_lock(&buffer.mutex);
    terminate = 1;
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.not_empty);
    pthread_cond_destroy(&buffer.not_full);

    return 0;
}
