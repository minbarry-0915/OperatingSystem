#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

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
        char *gcc_args[] = {"gcc", "-fsanitize=address", targetsrc, "-o", "compiled_program", NULL};
        execvp("gcc", gcc_args);
        // 성공하면 이후의 코드는 실행되지 않음

        // execvp가 실패했을 경우
        perror("fail: execvp");
        exit(1);
    } else if (pid > 0) {
        // 부모 프로세스
        int status;
        wait(&status); // 자식 프로세스가 종료될 때까지 대기
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);  //0이면 정상종료, 아니면 에러
            if (exit_status == 0) {
                printf("compile success\n");
            } else {
                fprintf(stderr, "compile error\n");
                exit(1);
            }
        }
    } else {
        // fork 실패
        perror("fail: fork");
        exit(1);
    }
}

void alarm_handler(int sig) {
    kill(getpid(), SIGKILL); //자식 프로세스 죽여라
}


void run_test_cases(char *inputdir, char *answerdir, int timelimit, char *compiled_program, int index) {
    if (index > MAX_TEST_CASES) {
        return;  // 모든 테스트 케이스를 완료했으므로 종료
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    signal(SIGALRM, alarm_handler);
    // 시그널 핸들러 등록

    char input_path[1024];
    char answer_path[1024];

    sprintf(input_path, "%s/%d.txt", inputdir, index); // input/1.txt
    sprintf(answer_path, "%s/%d.txt", answerdir, index); // answer/1.txt

    //input 읽기 전용으로
    int input_fd = open(input_path, O_RDONLY);
    if (input_fd < 0) {
        printf("No more test cases. Exiting...\n");
        return; // 함수 종료
    }

    // answer 읽기 전용으로
    int answer_fd = open(answer_path, O_RDONLY);
    if (answer_fd < 0) {
        perror("Runtime - cannot open answer file");
        exit(1);
    }

    // 자식 프로세스 생성
    pid_t pid = fork();


    if (pid == 0) { //자식 프로세스
        alarm(timelimit); // 타이머 설정 define timelimit 1
        close(pipefd[0]); //pipe읽기 닫기: 쓰기만

        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            // 표준 출력(STDOUT)을 파이프로 리다이렉트
            perror("dup2");
            exit(1);
        }
        close(pipefd[1]); // STDOUT으로 리다이렉트 했으므로 파이프의 쓰기 닫기


        dup2(input_fd, STDIN_FILENO);
        // 표준 입력(STDIN)을 파일로 리다이렉트

        // 즉 이 시점에서 파이프[1]은 자식 프로세스의 출력으로
        // input_fd는 자식 프로세스의 입력으로 사용됨

        close(input_fd);

        char *program_args[] = {compiled_program, NULL};
        // 여기서 NULL 이 들어간 이유는 execvp 함수가 인자를 받을 때 NULL로 끝나는 배열을 받기 때문
        execvp(compiled_program, program_args);
        // 이걸 성공적으로 돌리게 되면 출력이 pipe를 통해 부모 프로세스로 전달됨


        // execvp가 실패했을 경우
        perror("fail: execvp");

        exit(1);
    } else if (pid > 0) { // 부모 프로세스
        close(pipefd[1]);
        // 부모 프로세스에서는 파이프의 쓰기 닫기
        // 부모 프로세스는 자식 프로세스의 출력을 읽기만 함

        int status;

        struct timeval start_time, end_time;
        long long elapsed_time;

        // 프로그램 시작 시간 기록
        gettimeofday(&start_time, NULL);

        wait(&status); // 자식 프로세스가 종료될 때까지 대기
        printf("%d", WTERMSIG(status));
        // 프로그램 종료 시간 기록
        gettimeofday(&end_time, NULL);

        // 시간 차이 계산
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000LL + (end_time.tv_usec - start_time.tv_usec);


        // 결과 출력
        printf("\ntest case %d: ", index);

        if (WIFSIGNALED(status)) { // 신호에 의해서 종료되었는지를 판단, 시그널에 의해 종료되었을 때 참
            printf("%d", WTERMSIG(status));
            if (WTERMSIG(status) == SIGALRM) {
                // 시그널이 SIGALRM이면 timeout
                printf("Timeout Error\n");
                timeout_num++;
            } else{
                // 윈도우에서는 안돌아감
                // 시그널에 의해 종료되었지만 SIGALRM이 아닐 때, 시그널 번호 출력
                printf("Runtime Error - signal %d\n", WTERMSIG(status));
                runtime_error_num++;
            }
        } else {
            if (WIFEXITED(status)) {  // 자식 프로세스가 정상 종료 되었을때
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) { // 만약 정상종료 되었다면
                    // 정답과 비교
                    // 파이프로부터 자식 프로세스의 출력을 읽음
                    char output_buffer[4096];
                    ssize_t output_size = read(pipefd[0], output_buffer, sizeof(output_buffer));
                    // 파이프로부터 읽은 내용을 output_buffer에 저장
                    // output_size에는 읽은 바이트 수가 저장됨

                    // 못읽어와서 -1 반환받으면
                    if (output_size < 0) {
                        perror("runtime error - cannot read pipe"); // 에러
                        exit(1); // 종료
                    }
                    close(pipefd[0]); // 파이프 읽기 종료

                    char answer_buffer[4096];
                    int answer_fd = open(answer_path, O_RDONLY);
                    if (answer_fd < 0) {
                        perror("runtime error - cannot open answer file");
                        exit(1);
                    }
                    ssize_t answer_size = read(answer_fd, answer_buffer, sizeof(answer_buffer));
                    // 정답 파일을 읽어와서 answer_buffer에 저장
                    // answer_size에는 읽은 바이트 수가 저장됨


                    if (output_size != answer_size || memcmp(output_buffer, answer_buffer, output_size) != 0) {
                        // input과 output buffer 크기가 다르거나, 두 버퍼에 있는 값이 다르면

                        // 틀린 답이라 출력
                        printf("Fail - wrong answer\n");
                        wrong_num++;
                    } else {
                        // 그게 아니라면 정답이라 출력
                        printf("Success - correct answer\n");
                        correct_num++;
                        // 실행 시간 출력 (밀리초 단위)
                        printf("correct process runtime : %lld ms\n", elapsed_time / 1000);
                        printf("\n");
                        total_runtime += elapsed_time;
                    }
                }
            }
        }
        alarm(0); // 다음 테스트 케이스를 위해 알람 초기화
        close(input_fd);
        close(answer_fd);

        // 다음 테스트 케이스 실행
        run_test_cases(inputdir, answerdir, timelimit, compiled_program, index + 1);
    } else {
        // fork 실패
        perror("fail: fork");
        exit(1);
    }
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

    run_test_cases(inputdir, answerdir, timelimit, "./compiled_program", 1);

    // 결과 출력
    printf("\n\n ========== [RESULT] ========== \n");

    printf("Correct test case: %d cases\n", correct_num);
    printf("Wrong test case: %d cases\n", wrong_num);
    printf("Timeout test case: %d cases\n", timeout_num);
    printf("Runtime error test case: %d cases\n\n", runtime_error_num);

    printf("correct/total : %d / %d \n\n", correct_num, wrong_num + correct_num + timeout_num + runtime_error_num);

    printf("Total correct case runtime: %lld ms\n", total_runtime / 1000);
    printf("(When you run %d correct test cases)\n", correct_num);
    return 0;

}