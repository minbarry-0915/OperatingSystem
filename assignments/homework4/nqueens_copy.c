#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "stack.h"

// gcc -DBOARD_SIZE=15 stack.c nqueens.c -pthread
#ifndef BOARD_SIZE
#define BOARD_SIZE 15
#endif

#define BUFFER_SIZE 4
int find_n_queens_with_prepositions(int N, struct stack_t *prep);

typedef struct{
    struct stack_t *results[BUFFER_SIZE];
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
}buffer_t;

typedef struct{
    int size;
    int capacity;
    struct stack_t ** solutions;
    pthread_mutex_t mutex;
}final_solution_t;

buffer_t buffer;
final_solution_t final_solution;
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
    int board[BOARD_SIZE][BOARD_SIZE] ;
	int c, r ;

    for (r = 0 ; r < BOARD_SIZE ; r++) {
        for (c = 0 ; c < BOARD_SIZE ; c++) {
            board[r][c] = 0 ;
        }
    }

    for (int i = 0; i < get_size(queens); i++) {
        int cell;
        get_elem(queens, i, &cell);

        int r = row(cell);
        int c = col(cell);

        if (board[r][c] != 0) {
            return 0;
        }

        int x, y ;
		for (y = 0 ; y < BOARD_SIZE ; y++) {
			board[y][c] = 1 ;
		}
		for (x = 0 ; x < BOARD_SIZE ; x++) {
			board[r][x] = 1 ;
		}

		y = r + 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y += 1 ; x += 1 ;
		}

		y = r + 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y += 1 ; x -= 1 ;
		}

		y = r - 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y -= 1 ; x += 1 ;
		}

		y = r - 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y -= 1 ; x -= 1 ;
		}
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

int is_in_buffer(struct stack_t *queens, struct stack_t *q2){
     if (queens == NULL || q2 == NULL) {
        pthread_mutex_unlock(&buffer.mutex);
        return 0;
    }

    if(queens->size != q2->size){
        pthread_mutex_unlock(&buffer.mutex);
        return 0;
    }

    for(int i = 0; i < queens->size; i++){
        int elem_queen;
        int elem_buffer;
        get_elem(queens, i, &elem_queen);
        get_elem(q2, i, &elem_buffer);
        if(row(elem_queen) != row(elem_buffer) || col(elem_queen) != col(elem_buffer)){
            pthread_mutex_unlock(&buffer.mutex);
            return 0;
        }
    }

    return 1;
}

int is_in_solution(struct stack_t *solution){
    int found = 0;
    for(int i = 0; i < final_solution.size; i++){
        struct stack_t * current_solution = final_solution.solutions[i];
        found = is_in_buffer(solution, current_solution);
        if(found == 1)
            break;
    }
    if(found == 1)
        
        return 1;
    return 0;
}

void *producer(void *arg) {
    int N = *((int *)arg);
    struct stack_t *queens = create_stack(BOARD_SIZE);
    push(queens, 0);

    pthread_t tid = pthread_self();

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
            //버퍼에 혹시 같은 경로가 들어가 있는지 확인
            int is_dup = 0;
            pthread_mutex_lock(&buffer.mutex);
            for (int i = 0; i < buffer.count; i++) {
                int index = (buffer.out + i) % BUFFER_SIZE;
                if (is_in_buffer(queens, buffer.results[index])) {
                    is_dup = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&buffer.mutex);
            
            //스택에 좌표가 4개가 되면, 버퍼에 넘겨줌
            if (get_size(queens) == 4 && is_dup != 1) {
                printf("thread id: %ld\n", (unsigned long)tid);
                print_placement(queens);
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
                //4개를 찾으면 consumer에게 알림
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
        //꺼낼게 생성될 때 까지 대기
        while (buffer.count == 0 && !terminate) {
            pthread_cond_wait(&buffer.not_empty, &buffer.mutex);
        }

        if (terminate) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }
        //버퍼에 저장해둔 queens들 가져옴
        struct stack_t *queens = buffer.results[buffer.out];
        buffer.out = (buffer.out + 1) % BUFFER_SIZE;
        buffer.count--;

        //버퍼에 자리가 났음을 알려줌
        pthread_cond_signal(&buffer.not_full);
        pthread_mutex_unlock(&buffer.mutex);
        //가져온 queens들로 경로를 찾음
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
    pthread_t tid = pthread_self();

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
            if (get_size(queens) == N && !is_in_solution(queens)) {
                pthread_mutex_lock(&buffer.mutex);

                // Allocate memory for the new solution
                final_solution.solutions[final_solution.size] = create_stack(BOARD_SIZE);

                // Copy the contents of queens to final_solution->solutions[final_solution->size]
                final_solution.solutions[final_solution.size]->size = queens->size;
                final_solution.solutions[final_solution.size]->capacity = queens->capacity;
                memcpy(final_solution.solutions[final_solution.size]->buffer, queens->buffer, queens->size * sizeof(int));
                
                
                total_solutions++;
                pthread_mutex_unlock(&buffer.mutex);
                printf("%ld\n", (unsigned long)tid);
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
    
    final_solution.size = 0;
    final_solution.capacity = 1000;
    final_solution.solutions = calloc(sizeof(struct stack_t*),final_solution.capacity);
    pthread_mutex_init(&final_solution.mutex, NULL);

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
    pthread_cond_signal(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.not_empty);
    pthread_cond_destroy(&buffer.not_full);

    return 0;
}
