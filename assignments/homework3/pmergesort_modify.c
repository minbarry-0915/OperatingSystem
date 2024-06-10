#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define max_tasks 200 

struct timeval start_time, end_time; // 프로세스 시간 체크용

double* data;
int n_data = 0;

enum task_status { UNDONE, PROCESS, DONE };

// 정렬 작업 구조체
struct sorting_task {
	double* a;
	int n_a;
	int status;
};

struct sorting_task tasks[max_tasks];
int n_tasks = 0;
int n_undone = 0;
int n_done = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

// 리스트 병합 함수
void merge_lists(double* a1, int n_a1, double* a2, int n_a2);
// 병합 정렬 함수
void merge_sort(double* a, int n_a);

// 작업 스레드 함수
void* worker(void* ptr) {
	while (1) {
		//현재 스레드가 접근하고 있는 메모리 접근 못하게 잠금
		pthread_mutex_lock(&m);
		//할게 없으면
		while (n_undone == 0) {
			//메인 스레드에서 작업 만들어서 신호줄때 까지 기다림
			pthread_cond_wait(&cv, &m);
		}

		//할게 생김
		//작업 진행
		int i;
		for (i = 0; i < n_tasks; i++) {
			//작업 안된 곳 찾음
			if (tasks[i].status == UNDONE) 
				break;
		}

		tasks[i].status = PROCESS;
		n_undone--;
		//잠금 해제
		pthread_mutex_unlock(&m);

		printf("[Thread %ld] starts Task %d\n", pthread_self(), i);

		merge_sort(tasks[i].a, tasks[i].n_a);

		printf("[Thread %ld] completed Task %d\n", pthread_self(), i);
		
		pthread_mutex_lock(&m);
		tasks[i].status = DONE;
		n_done++;
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

// 데이터 초기화 스레드 함수
void* init_data_worker(void* ptr) {
	//첫번째 param
	int thread_id = *(int*)ptr;
	//두번째 param
	int n_threads = *(int*)(ptr + sizeof(int));

	// 작업 시작할 주소
	int start = thread_id * (n_data / n_threads);
	int end;
	// 마지막 스레드이면
	if(thread_id == n_threads - 1){
		// 끝 주소가 데이터의 끝
		end = n_data;
	}
	else{
		// 아니면 끝 주소가 데이터 시작주소 더하기 스레드당 데이터양
		end = start + (n_data/n_threads);
	}


	// 난수 생성 및 data 초기화
	struct timeval ts;
	gettimeofday(&ts, NULL);
	srand(ts.tv_usec * ts.tv_sec * thread_id);

	for (int i = start; i < end; i++) {
		int num = rand();
		int den = rand();
		if (den != 0.0)
			data[i] = ((double)num) / ((double)den);
		else
			data[i] = ((double)num);
	}
	return NULL;
}

// 데이터 병합 스레드 함수
void* merge_data_worker(void* ptr) {
    // 파라미터 해체
    int curr_thread = *((int*)ptr);  // 몇번째 스레드인지
    int n_tasks = *((int*)(ptr + sizeof(int)));  // 총 작업 스텝 수
    int n_threads = *((int*)(ptr + 2 * sizeof(int)));  // 총 스레드 수

    int tasks_per_thread = n_tasks / n_threads; // 스레드당 처리할 작업 수
    int extra_tasks = n_tasks % n_threads; // 남은 작업 수

    // 병합할 작업 범위 계산
    int start_idx = curr_thread * tasks_per_thread; // 작업 시작 인덱스
    int end_idx = start_idx + tasks_per_thread + (curr_thread == n_threads - 1 ? extra_tasks : 0); // 작업 끝 인덱스

	int data_per_thread = n_data / n_threads + (curr_thread == n_threads - 1? n_data % n_threads: 0);

	double* start = data + curr_thread * data_per_thread;
    // 병합 작업 수행
    int n_sorted = 0;
    for (int i = start_idx; i < end_idx; i++) {
        merge_lists(start, n_sorted, tasks[i].a, tasks[i].n_a);
        n_sorted += tasks[i].n_a;	
    }
    return NULL;
}

int main(int argc, char* argv[]) {
	// 시작 시간 측정
	gettimeofday(&start_time, NULL);

	// 스레드 갯수
	int n_threads = 0;
	
	int opt;
	while ((opt = getopt(argc, argv, "d:t:")) != -1) {
		switch (opt) {
		case 'd':
			//옵션으로 받아온 생성할 데이터 갯수
			n_data = atoi(optarg);
			if (n_data <= 0) {
				fprintf(stderr, "invalid number\n");
				return EXIT_FAILURE;
			}
			break;
		case 't':
			//옵션으로 받아온 생성할 스레드 갯수
			n_threads = atoi(optarg);
			if (n_threads <= 0) {
				fprintf(stderr, "invalid number\n");
				return EXIT_FAILURE;
			}
			break;

		default:
			return EXIT_FAILURE;
		}
	}

	// 데이터 메모리 할당
	data = (double*)malloc(n_data * sizeof(double));
	if (data == NULL) {
		fprintf(stderr, "Fail to allocate\n");
		return EXIT_FAILURE;
	}

	// 데이터 초기화 스레드 생성
	pthread_t init_threads[n_threads];
	// 스레드에 넘겨줄 인자들
	int thread_params[n_threads][2];
	for (int i = 0; i < n_threads; i++) {
		thread_params[i][0] = i; //thread id
		thread_params[i][1] = n_threads; //스레드 개수

		//스레드당 처리할 데이터를 스레드를 이용하여 병렬로 배정하기 때문에 초기화 시간이 줄어듬
		pthread_create(&init_threads[i], NULL, init_data_worker, (void*)thread_params[i]);
	}
	//데이터 초기화 완료될때 까지 대기
	for (int i = 0; i < n_threads; i++) {
		pthread_join(init_threads[i], NULL);
	}

	// 정렬 작업 스레드 생성
	pthread_t sort_threads[n_threads];
	for (int i = 0; i < n_threads; i++) {
		pthread_create(&sort_threads[i], NULL, worker, NULL);
	}

	// 작업당 처리해야되는 데이터
	int data_per_task = n_data / max_tasks;
	// 데이터가 딱 안떨어질 수 있음
	int extra_tasks = n_data % max_tasks;

	for (int i = 0; i < max_tasks; i++) {
		pthread_mutex_lock(&m);
		//데이터 시작주소
		tasks[i].a = data + data_per_task * i;
		//데이터 길이
		tasks[i].n_a = data_per_task;
		//마지막 작업이면
		if (i == max_tasks - 1)
			//남은 작업을 마지막작업에 더해줌
			tasks[i].n_a += extra_tasks;
		//모든 작업은 아직 완료되지 않았으므로 undone으로 초기화
		tasks[i].status = UNDONE;

		//개수 추가
		n_undone++;
		n_tasks++;

		//스레드에 신호보내서 작업시작하라고 함
		pthread_cond_signal(&cv);
		pthread_mutex_unlock(&m);
	}

	// 메인 스레드는 모든 작업이 끝날 때까지 대기
	// n_done 변수를 검사할때 다른 스레드가 수정하지 않도록 잠금
	pthread_mutex_lock(&m);
	while (n_done < max_tasks) {
		//작업이 끝난 스레드가 n_done을 수정할 수 있도록 잠금 해제 
		pthread_mutex_unlock(&m);
		//다시 못 수정 하게 잠금
		pthread_mutex_lock(&m);

	}
	//모든 작업이 종료 되었으므로 잠금 해제 
	pthread_mutex_unlock(&m);

	//병합 작업 병렬 처리
	//int merge_steps = n_threads;
	pthread_t merge_threads[n_threads];
	int merge_params[n_threads][3];

	for (int i = 0; i < n_threads; i++) {
		//병합 함수에 전해줄 파라미터
		merge_params[i][0] = i;
		merge_params[i][1] = max_tasks;
		merge_params[i][2] = n_threads;
		pthread_create(&merge_threads[i], NULL, merge_data_worker, (void*)merge_params[i]);
	}
	for (int i = 0; i < n_threads; i++) {
		pthread_join(merge_threads[i], NULL);
	}


	int i; 
	int data_per_thread = n_data / n_threads;
	int n_sorted = data_per_thread;

	int next_n_data;
	for(i = 1; i < n_threads; i++){
		double* next = data + i * data_per_thread;
		if(i == n_threads - 1){
			next_n_data = data_per_thread + n_data % n_threads;
		}
		else{
			next_n_data = data_per_thread;
		}
		merge_lists(data, n_sorted, next, next_n_data);
		n_sorted += next_n_data;
	}

	// 종료 시간 측정
	gettimeofday(&end_time, NULL);
	double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

	printf("Total time: %.5f seconds\n", total_time);

#ifdef DEBUG
	for (int i = 0; i < n_data; i++) {
		printf("%lf ", data[i]);
	}
	printf("\nTotal time: %.5f seconds\n", total_time);
#endif

	free(data);
	return EXIT_SUCCESS;
}

// 리스트 병합 함수
void merge_lists(double* a1, int n_a1, double* a2, int n_a2) {
	double* a_m = (double*)calloc(n_a1 + n_a2, sizeof(double));
	int i = 0;

	int top_a1 = 0;
	int top_a2 = 0;

	for (i = 0; i < n_a1 + n_a2; i++) {
		if (top_a2 >= n_a2) {
			a_m[i] = a1[top_a1];
			top_a1++;
		} else if (top_a1 >= n_a1) {
			a_m[i] = a2[top_a2];
			top_a2++;
		} else if (a1[top_a1] < a2[top_a2]) {
			a_m[i] = a1[top_a1];
			top_a1++;
		} else {
			a_m[i] = a2[top_a2];
			top_a2++;
		}
	}
	memcpy(a1, a_m, (n_a1 + n_a2) * sizeof(double));
	free(a_m);
}

// 병합 정렬 함수
void merge_sort(double* a, int n_a) {
	if (n_a < 2)
		return;

	double* a1;
	int n_a1;
	double* a2;
	int n_a2;

	a1 = a;
	n_a1 = n_a / 2;

	a2 = a + n_a1;
	n_a2 = n_a - n_a1;

	merge_sort(a1, n_a1);
	merge_sort(a2, n_a2);

	merge_lists(a1, n_a1, a2, n_a2);
}
