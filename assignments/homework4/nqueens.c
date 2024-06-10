#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

// gcc -DBOARD_SIZE=15 stack.c nqueens.c
#ifndef BOARD_SIZE
#define BOARD_SIZE	15
#endif 


int row (int cell)
{
	return cell / BOARD_SIZE ;
}

int col (int cell)
{
	return cell % BOARD_SIZE ;
}

int is_feasible (struct stack_t * queens) 
{
		int board[BOARD_SIZE][15] ;
		int c, r ;

		for (r = 0 ; r < BOARD_SIZE ; r++) {
			for (c = 0 ; c < BOARD_SIZE ; c++) {
				board[r][c] = 0 ;
			}
		}

	for (int i = 0 ; i < get_size(queens) ; i++) {
		int cell ;
		get_elem(queens, i, &cell) ;
		
		int r = row(cell) ;
		int c = col(cell) ;
	
		if (board[r][c] != 0) {
			return 0 ;
		}

		int x, y ;
		for (y = 0 ; y < BOARD_SIZE ; y++) {
			board[y][c] = 1 ;
		}
		for (x = 0 ; x < BOARD_SIZE ; x++) {
			board[r][x] = 1 ;
		}

		y = r + 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < 15) {
			board[y][x] = 1 ;
			y += 1 ; x += 1 ;
		}

		y = r + 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < 15) {
			board[y][x] = 1 ;
			y += 1 ; x -= 1 ;
		}

		y = r - 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < 15) {
			board[y][x] = 1 ;
			y -= 1 ; x += 1 ;
		}

		y = r - 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < 15) {
			board[y][x] = 1 ;
			y -= 1 ; x -= 1 ;
		}

	}

	return 1;
}

void print_placement (struct stack_t * queens)
{
	for (int i = 0 ; i < queens->size ; i++) {	
		int queen ;
		get_elem(queens, i, &queen) ;
		printf("[%d,%d] ", row(queen), col(queen)) ;
	}
}

//퀸이 진행했던 과거 경로를 받고 나머지 길을 찾음
int find_n_queens_with_prepositions (int N, struct stack_t * prep)
{
	struct stack_t * queens = create_stack(BOARD_SIZE) ;

	queens->capacity = prep->capacity ;
	queens->size = prep->size ;
	memcpy(queens->buffer, prep->buffer, prep->size * sizeof(int)) ;

	while (!is_empty(queens)) {
		int latest_queen ;
		top(queens, &latest_queen) ;

		if (latest_queen == BOARD_SIZE * 15) {
			pop(queens, &latest_queen) ;
			if (!is_empty(queens)) {
				pop(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
			else {
				break ;
			}
			continue ;
		}

		if (is_feasible(queens)) {
			//길을 을 하나 찾았을때
			if (get_size(queens) == N) {
			
				print_placement(queens) ;

				printf("\n") ;

				int lastest_queen ;
				pop(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
			else {
				int latest_queen ;
				top(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
		}
		else {
			int lastest_queen ;
			pop(queens, &latest_queen) ;
			push(queens, latest_queen + 1) ;
		}

	}
	delete_stack(queens) ;
}


int find_n_queens (int N)
{
	struct stack_t * queens = create_stack(BOARD_SIZE) ;

	push(queens, 0) ;
	while (!is_empty(queens)) {
		int latest_queen ;
		top(queens, &latest_queen) ;

		if (latest_queen == BOARD_SIZE * 15) {
			pop(queens, &latest_queen) ;
			if (!is_empty(queens)) {
				pop(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
			else {
				break ;
			}
			continue ;
		}

		if (is_feasible(queens)) {
			if (get_size(queens) == N) {

				print_placement(queens) ;
				printf("\n") ;

				int lastest_queen ;
				pop(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
			else {
				int latest_queen ;
				top(queens, &latest_queen) ;
				push(queens, latest_queen + 1) ;
			}
		}
		else {
			int lastest_queen ;
			pop(queens, &latest_queen) ;
			push(queens, latest_queen + 1) ;
		}

	}
	delete_stack(queens) ;
}
//스레드 갯수 8개정도 64보다는 작게 해야됨
//bounded buffer서야됨
//길찾으면 화면에 뿌려야됨
//동시에 프린팅 겹치지 않게 처리해야됨
//컨 c누르면 지금까지 찾은 경로 총갯수 출력해야되고 종료하면됨
//bounded buffer어떻게 썻는지 영상에 포함되어야함
//make file이랑 readme 만들어야됨
//실패해도 어디까지 했는지 말해달라ㅏㅏㅏㅏㅏ

int main ()
{
	find_n_queens(BOARD_SIZE) ;
	return EXIT_FAILURE ;
}
