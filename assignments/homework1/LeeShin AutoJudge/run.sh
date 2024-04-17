#!/bin/bash

# 변수로 입력 받기
inputdir=$1
outputdir=$2
timelimit=$3
targetsrc=$4

# OS 확인
OS=$(uname -s)

if [ "$OS" = "Darwin" ]; then
    # 맥 환경일 경우
    gcc submit.c -o submit
elif [ "$OS" = "Linux" ]; then
    # 리눅스 환경일 경우
    gcc submit2.c -o submit
else
    echo "Unsupported OS"
    exit 1
fi

# 변수를 사용하여 autojudge 실행
./submit -i "$inputdir" -a "$outputdir" -t "$timelimit" "$targetsrc"
