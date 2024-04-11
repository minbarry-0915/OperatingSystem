#!/bin/bash

# 변수로 입력 받기
inputdir=$1
outputdir=$2
timelimit=$3
targetsrc=$4

# submit.c 컴파일
gcc submit.c -o submit

# 변수를 사용하여 autojudge 실행
./submit -i "$inputdir" -a "$outputdir" -t "$timelimit" "$targetsrc"
