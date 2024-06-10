# 프로젝트 이름
pmergesort 
해당 프로그램은 난수 생성후에 해당 난수를 멀티 스레드를 이용하여 병렬로 정렬합니다.

데이터 가 난수로 생성되므로 실행 결과에 따라 실행시간이 달라집니다. 
## 사용법
-컴파일
gcc -o pmergesort pmergesort_modify.c -pthread

-실행
./pmergesort -d 데이터 수 -t 스레드 수 
스레드 수와 데이터 수는 모두 정수 여야하며, 0보다 커야합니다.
스레드 수는 짝수여야하며, 10 이하여야합니다.

## 기여
충북대학교 토목공학부 이지민