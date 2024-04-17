#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>

#define MAX_TEST_CASES 20
#define TIME_LIMIT 1


int correct_num = 0;
int timeout_num = 0;
int wrong_num = 0;
int runtime_error_num = 0;
long long total_runtime = 0;

void parse_arguments(int argc, char *argv[], char **inputdir, char **outputdir, int *timelimit, char **targetsrc) {
    int option;
    while ((option = getopt(argc, argv, "i:a:t:")) != -1) {
        switch (option) {
            case 'i':
                *inputdir = optarg;
                break;
            case 'a':
                *outputdir = optarg;
                break;
            case 't':
                *timelimit = atoi(optarg);
                break;
            default:
                fprintf(stderr, "잘못된 인자입니다.\n");
                exit(1);
        }
    }

    if (optind < argc) {
        *targetsrc = argv[optind];
    } else {
        fprintf(stderr, "채점할 C 파일을 지정해야 합니다.\n");
        exit(1);
    }
}

void compile_source(char *targetsrc) {
    pid_t pid = fork();

    if (pid == 0) {
        // 자식 프로세스에서 컴파일을 실행
        char *gcc_args[] = {"gcc", "-fsanitize=address", "-o", "compiled_program", targetsrc , NULL};
        execvp("gcc", gcc_args);
        // execvp가 실패했을 경우
        perror("fail: execvp\n");
        exit(1);
    } else if (pid > 0) {
        // 부모 프로세스
        int status;
        wait(&status); // 자식 프로세스가 종료될 때까지 대기
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);  //0이면 정상종료, 아니면 에러
            if (exit_status == 0) {
                printf("compile success\n\n");
            } else {
                fprintf(stderr, "compile error\n");
                exit(1);
            }
        }
    } else {
        // fork 실패
        perror("fail: fork\n");
        exit(1);
    }
}

void alarm_handler(int sig) {
    kill(getpid(),SIGKILL); //자식 프로세스 죽여라
}

void run_test_cases(char* input_path, char* answer_path, int timelimit, char *compiled_program) {

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe\n");
        exit(1);
    }


    signal(SIGALRM, alarm_handler);

    //input 읽기 전용으로
    printf("input_path: %s\n",input_path);
    int input_fd = open(input_path, O_RDONLY);  
    if (input_fd < 0) {
        printf("No more test cases. Exiting...\n");
        return; // 함수 종료
    }

    // answer 읽기 전용으로
    printf("answer_path: %s\n",answer_path);
    int answer_fd = open(answer_path, O_RDONLY);  
    if (answer_fd < 0) {
        perror("Runtime - cannot open answer file\n");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스에서 프로그램 실행
        alarm(timelimit); // 타이머 설정 define timelimit 1
       
        close(pipefd[0]); //pipe읽기 닫기:쓰기만


        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {  //stdout을 파이프로 리다이렉트: 이제 출력된게 파이프로 감
            perror("dup2\n");
            exit(1);
        }

        close(pipefd[1]); // STDOUT으로 리다이렉트 했으므로 파이프의 쓰기 닫기

        dup2(input_fd, STDIN_FILENO);  
        close(input_fd);
        
    
        char *program_args[] = {compiled_program, NULL};
        execvp(compiled_program, program_args);
                 
        //execvp가 실패했을 경우
        perror("fail: execvp\n");

        exit(1);
    } else if (pid > 0) {
        // 부모 프로세스
        close(pipefd[1]); // 부모 프로세스에서는 파이프의 쓰기 닫기
        
        int status;
        
        struct timeval start_time, end_time;
        long long elapsed_time;

         // 프로그램 시작 시간 기록
        gettimeofday(&start_time, NULL);

        wait(&status); // 자식 프로세스가 종료될 때까지 대기
 
        // 프로그램 종료 시간 기록
        gettimeofday(&end_time, NULL);

        // 시간 차이 계산
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000LL + (end_time.tv_usec - start_time.tv_usec);

        printf("\033[31m\ntest case %s: \033[0m", input_path);
        
        if (WIFSIGNALED(status)) {  //자식프로세스가 시그널에 의해서 종료되었을때 ex 무한루프
            if (WTERMSIG(status) == SIGALRM) {
                // 시그널이 SIGALRM이면 timeout
                printf("Timeout Error\n\n");
                timeout_num++;
                } 
        }    
        else{
            if (WIFEXITED(status)) {  //자식프로세스가 정상 종료 되었고 
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) { //exit code가 0일때 
                    //정답과 비교                
                    // 파이프로부터 자식 프로세스의 출력을 읽음
                    char output_buffer[4096];
                    ssize_t output_size = read(pipefd[0], output_buffer, sizeof(output_buffer));
                    
                    //못읽어와서 -1 반환받으면
                    if (output_size < 0) {
                           perror("runtime error - cannot read pipe\n"); //에러
                        exit(1); //종료
                    }
                    close(pipefd[0]); // 파이프 읽기 종료
                    
                    char answer_buffer[4096];
                    int answer_fd = open(answer_path, O_RDONLY);
                    if (answer_fd < 0) {
                        perror("runtime error - cannot open answer file\n");
                        exit(1);
                    }          
                    ssize_t answer_size = read(answer_fd, answer_buffer, sizeof(answer_buffer));

                    if (output_size != answer_size || memcmp(output_buffer, answer_buffer, output_size) != 0) {
                        printf("Fail - wrong answer\n\n");
                        //카운트 해야됨
                        wrong_num ++;
                    } else {
                        printf("Success - correct answer\n");
                        //카운트 해야됨
                        correct_num ++;
                        // 실행 시간 출력 (밀리초 단위)
                        printf("correct process runtime : %lld ms\n", elapsed_time / 1000);
                        printf("\n");
                        total_runtime += elapsed_time;
                    }
                } else { //자식프로세스가 정상 종료되었지만 exit code가 0이 아닐때 ex) segmentation fault, buffer overflow.
                    //원래는 시그널에 의해 종료되고 exitcode가 0이 아니여야하지만, 이 운영체제에서는 정상종료되고 exitcode를 반환하지 않고 있다..ㅠㅠ
                    printf("runtime error - exit code is not null %d\n\n", WTERMSIG(status));
                    runtime_error_num++;
                }
            }
        }

        alarm(0); // 다음 테스트 케이스를 위해 알람 초기화
        close(input_fd);
        close(answer_fd);

    }
    else {
        // fork 실패
        perror("fail: fork");
        exit(1);
    }
}

int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}


int main(int argc, char *argv[]) {
    char *inputdir = NULL; //입력 디렉토리
    char *answerdir = NULL; //정답 디렉토리
    int timelimit = TIME_LIMIT; //시간제한
    char *targetsrc = NULL; //채점할 소스파일 

    parse_arguments(argc, argv, &inputdir, &answerdir, &timelimit, &targetsrc);

    printf("Input Directory: %s\n", inputdir);
    printf("Answer Directory: %s\n", answerdir);
    printf("Time Limit: %d sec\n", timelimit);
    printf("Target SourceFile to grade: %s\n", targetsrc);

    compile_source(targetsrc);

    printf("=========== Running test cases ============\n");

    // 여기에서 추가적인 채점 로직을 구현
    DIR *dir;
    struct dirent *ent;
    char *input_files[1024]; // 파일명을 저장할 배열
    char *answer_files[1024]; // 파일명을 저장할 배열
    int input_count = 0;
    int answer_count = 0;

    //디렉토리 안의 모든 파일들을 처리
    if ((dir = opendir(inputdir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) { // 일반 파일인 경우만 처리
                input_files[input_count] = strdup(ent->d_name);
                input_count++;
            }
        }
        closedir(dir);

        qsort(input_files, input_count, sizeof(char *), compare);

    } else {
        perror("Error opening directory");
        return EXIT_FAILURE;
    }
    if ((dir = opendir(answerdir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) { // 일반 파일인 경우만 처리
                answer_files[answer_count] = strdup(ent->d_name);
                answer_count++;
            }
        }
        closedir(dir);
        qsort(answer_files, answer_count, sizeof(char *), compare);
    } else {
        perror("Error opening directory");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < answer_count; i++) {
            char input_path[1024];
            char answer_path[1024];
            snprintf(input_path, sizeof(input_path), "%s/%s", inputdir, input_files[i]);
            snprintf(answer_path, sizeof(answer_path), "%s/%s", answerdir,answer_files[i]);
            run_test_cases(input_path,answer_path,timelimit,"./compiled_program");
            free(input_files[i]);
            free(answer_files[i]); // strdup으로 할당된 메모리 해제
    }
    

    // 결과 출력
    printf("\033[36m\n\n ========== [RESULT] ========== \n\033[0m");

    printf("\033[32mCorrect test case: %d cases\n", correct_num);
    printf("\033[31mWrong test case: %d cases\n", wrong_num);
    printf("\033[33mTimeout test case: %d cases\n", timeout_num);
    printf("\033[35mRuntime error test case: %d cases\n\n\033[0m", runtime_error_num);

    printf("\033[34mcorrect/total : %d / %d \n\n\033[0m", correct_num,
           wrong_num + correct_num + timeout_num + runtime_error_num);

    printf("\033[32mTotal correct case runtime: %lld ms\n", total_runtime / 1000);
    printf("(When you run %d correct test cases)\n\033[0m", correct_num);
    return 0;
}