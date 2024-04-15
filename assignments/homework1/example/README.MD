# Lee Shin Judge

---

### 제작자
2017024158 이지민

2020015029 신현욱

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

## 참고 사항
1. 해당 프로그램은 리눅스 환경에서 실행되어야 합니다.
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