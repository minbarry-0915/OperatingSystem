# Lee Shin Judge

---

### 제작자
2017024158 이지민

2020015029 신현욱

## 영상 자료
[유튜브로 바로가기](https://youtu.be/qxhcJ1EloCw)

참고 : 해당 영상은 맥 환경에서 실행되었습니다.

## 실행 방법

run.sh을 다음과 같이 실행 시킵니다.


``` shell
chmod +x run.sh
```

1. 쉘스크립트에 실행 권한 추가합니다.


``` shell
./run.sh input answer 1 pathname.c
# ./run.sh <입력 폴더> <정답 폴더> <시간 제한> <테스트 할 C파일>
```

2. 해당 형식에 맞춰 쉘 스크립트를 실행 시킵니다.
   1. 시간 제한의 단위는 초 입니다.
   2. 만약 주소를 입력하지 않을 시, 현재 디렉토리를 기준으로 실행됩니다.
   3. 폴더 input과 answer안의 파일은 반드시 1.txt의 형식으로 지정되어있어야합니다.
   4. 또한 파일들의 인덱스는 1부터 시작해야하고 연속적이어야합니다.

## 첨부된 파일 설명
1. run.sh : 실행 스크립트
2. submit.c : 맥에서의 소스파일
3. submit2.c : 리눅스에서 소스파일
4. input : 테스트 케이스 입력 파일
5. answer : 테스트 케이스 정답 파일
6. pathname.c : 테스트 할 소스파일
7. pathname2.c : 문법 오류(컴파일 에러)가 있는 소스파일
8. README.md : 프로그램 설명서


## 참고 사항
1. 해당 프로그램은 리눅스/맥 환경에서 실행되어야 합니다.
2. 소스 프로그램은 C파일이어야 합니다.
3. gcc -fsanitize가 감지해주는 에러는 아래의 test 케이스에 해당됩니다.


ex)
```
test case 7: Timeout Error
AddressSanitizer:DEADLYSIGNAL
=================================================================
   ==7270==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x000104707ba8 bp 0x00016b6fa230 sp 0x00016b6fa1b0 T0)

~~~

test case 8: Runtime Error - signal 6
```
위와 같이 출력 될 경우, test case 8에 해당 되는 오류입니다.
4. 과제 요구사항에서 나온 커맨드 라인 인터페이스의 경우, run.sh에 아래와 같이 구현되어있습니다.
``` shell
   ./submit -i "$inputdir" -a "$outputdir" -t "$timelimit" "$targetsrc"
```
5. 리눅스와 맥환경 코드가 따로 있는 이유는, 똑같은 코드가 두 환경에서 동작하지 않았기 때문입니다. 차이가 있는 부분은 다음과 같습니다.

맥 환경에서는
``` c
        if (WIFSIGNALED(status)) { // 신호에 의해서 종료되었는지를 판단, 시그널에 의해 종료되었을 때 참
            if (WTERMSIG(status) == SIGALRM) {
                // 시그널이 SIGALRM이면 timeout
                printf("Timeout Error\n");
                timeout_num++;
            } else {
                // 시그널에 의해 종료되었지만 SIGALRM이 아닐 때, 시그널 번호 출력
                printf("Runtime Error - signal %d\n", WTERMSIG(status));
                runtime_error_num++;
            }
            
```
다음과 같은 코드로 런타임 오류와 타임아웃 에러를 구분하였습니다.

하지만 리눅스에서 이와 같은 방식으로는 런타임 오류 부분이 출력되지 않아.

리눅스에서는 다음과 같은방식으로 처리하였습니다.

``` c
        if (WIFSIGNALED(status)) {  
            //자식프로세스가 시그널에 의해서 종료되었을때 ex 무한루프
            if (WTERMSIG(status) == SIGALRM) {
                // 시그널이 SIGALRM이면 timeout
                printf("Timeout Error\n");
                timeout_num++;
                } 
        }    
        else {
            if (WIFEXITED(status)) {  //자식프로세스가 정상 종료 되었고 
            
               // 기존 코드
             
                } else { 
                
                //   이부분이 추가된 부분입니다.
                
                    printf("runtime error - exit code is not null %d\n", WTERMSIG(status));
                    runtime_error_num++;
                }
```

submit.c와 submit2.c는 다음과 같은 차이점만 있고, 나머지는 동일합니다.