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
        char *gcc_args[] = {"gcc", "-fsanitize=address", targetsrc,  "-o", "compiled_program", NULL};
        execvp("gcc", gcc_args);
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
    kill(getpid(),SIGKILL); //자식 프로세스 죽여라
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

    char input_path[1024];
    //char output_path[1024];
    char answer_path[1024];

    sprintf(input_path, "%s/%d.txt", inputdir, index); // input/1.txt
    //sprintf(output_path, "output/%d.txt", index); // output/1.txt
    sprintf(answer_path, "%s/%d.txt",answerdir, index); // answer/1.txt

    //input 읽기 전용으로
    int input_fd = open(input_path, O_RDONLY);  
    if (input_fd < 0) {
        printf("No more test cases. Exiting...\n");
        return; // 함수 종료
    }

    //output 쓰기 전용,파일이 없으면 생성가능,이미 있으면 덮어쓰기
    // int output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
    // if (output_fd < 0) {
    //     perror("Runtime - cannot open output file");
    //     exit(1);
    // }

    // answer 읽기 전용으로
    int answer_fd = open(answer_path, O_RDONLY);  
    if (answer_fd < 0) {
        perror("Runtime - cannot open answer file");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스에서 프로그램 실행
        alarm(timelimit); // 타이머 설정 define timelimit 1
        //struct itimerval timer; 
        // //받은 밀리세컨드 단위의 timelimit 변환
        // timer.it_value.tv_sec = timelimit / 1000; 
        // timer.it_value.tv_usec = (timelimit % 1000) / 1000;
        // timer.it_interval.tv_sec = 0; // 반복 없음
        // timer.it_interval.tv_usec = 0; // 마이크로초는 0

        // // 타이머 설정
        // if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        //     perror("setitimer");
        //     exit(1);
        // }
        close(pipefd[0]); //pipe읽기 닫기:쓰기만

        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {  //stdout을 파이프로 리다이렉트: 이제 출력된게 파이프로 감
            perror("dup2");
            exit(1);
        }
        close(pipefd[1]); // STDOUT으로 리다이렉트 했으므로 파이프의 쓰기 닫기


        //printf("\ntesting %d.txt ....\n", index);
        dup2(input_fd, STDIN_FILENO);  
        //dup2(output_fd, STDOUT_FILENO);
        close(input_fd);
        //close(output_fd);
    
        char *program_args[] = {compiled_program, NULL};
        execvp(compiled_program, program_args);
                 
        // execvp가 실패했을 경우
        perror("fail: execvp");

        exit(1);
    } else if (pid > 0) {
        // 부모 프로세스
        close(pipefd[1]); // 부모 프로세스에서는 파이프의 쓰기 닫기
        
        int status;
        
        struct timeval start_time, end_time;
        long long elapsed_time;

         // 프로그램 시작 시간 기록
        gettimeofday(&start_time, NULL);
        //printf("시작시간 : %ld\n",start_time.tv_usec);

        wait(&status); // 자식 프로세스가 종료될 때까지 대기
 
        // 프로그램 종료 시간 기록
        gettimeofday(&end_time, NULL);
        //printf("종료시간: %ld\n",end_time.tv_usec);

        // 시간 차이 계산
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000LL + (end_time.tv_usec - start_time.tv_usec);

        printf("test case %d: ", index);
        
        if (WIFSIGNALED(status)) {  //자식프로세스가 시그널에 의해서 종료되었을때 ex 무한루프
            printf("runtime error - timeout\n");
            timeout_num ++;
        } else{
            if (WIFEXITED(status)) {  //자식프로세스가 정상 종료 되었고 exit code도 0일때 
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) {
                    //정답과 비교                
                    // 파이프로부터 자식 프로세스의 출력을 읽음
                    char output_buffer[4096];
                    ssize_t output_size = read(pipefd[0], output_buffer, sizeof(output_buffer));
                    
                    //못읽어와서 -1 반환받으면
                    if (output_size < 0) {
                           perror("runtime error - cannot read pipe"); //에러
                        exit(1); //종료
                    }
                    close(pipefd[0]); // 파이프 읽기 종료
                    
                    char answer_buffer[4096];
                    int answer_fd = open(answer_path, O_RDONLY);
                    if (answer_fd < 0) {
                        perror("runtime error - cannot open answer file");
                        exit(1);
                    }          
                    //printf("Buffer contents: %s\n", output_buffer);
                    //ssize_t output_size = read(output_fd, output_buffer, sizeof(output_buffer));
                    ssize_t answer_size = read(answer_fd, answer_buffer, sizeof(answer_buffer));

                    //printf("%ld %ld\n", output_size, answer_size);

                    if (output_size != answer_size || memcmp(output_buffer, answer_buffer, output_size) != 0) {
                        printf("Fail - wrong answer\n");
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
                    printf("runtime error - exit code is not null\n");
                }
            }
        }


        alarm(0); // 다음 테스트 케이스를 위해 알람 초기화
        close(input_fd);
        //close(output_fd);
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

    // 여기에서 추가적인 채점 로직을 구현
    run_test_cases(inputdir, answerdir, timelimit, "./compiled_program", 1);

    // 결과 출력
    printf("\n\n ========== [RESULT] ========== \n");

    printf("Correct test case: %d cases\n", correct_num);
    printf("Wrong test case: %d cases\n", wrong_num);
    printf("Timeout test case: %d cases\n\n", timeout_num);

    printf("correct/total : %d / %d \n\n", correct_num, wrong_num + correct_num + timeout_num);

    printf("Total correct case runtime: %lld ms\n", total_runtime/1000);
    printf("(When you run %d correct test cases)\n", correct_num);
    return 0;
}