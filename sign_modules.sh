#!/bin/bash

# 인자 확인
if [[ $# -ne 1 ]]; then
  echo "사용법: $0 <모듈_이름.ko>"
  exit 1
fi

MODULE_NAME="$1"

# 모듈 파일 존재 확인
if [[ ! -f "${MODULE_NAME}" ]]; then
  echo "모듈 파일을 찾을 수 없습니다: ${MODULE_NAME}"
  exit 1
fi

# 키 생성 (OpenSSL 사용)
echo "개인 키 및 공개 키 생성..."
openssl genrsa -out private.key 2048
openssl req -new -x509 -key private.key -out public.der -days 3650 -subj "/CN=My Kernel Module Key"

if [[ $? -ne 0 ]]; then
  echo "키 생성 실패!"
  exit 1
fi

echo "개인 키 (private.key) 및 공개 키 (public.der) 생성 완료."

# 커널 소스 코드 경로 확인
KERNEL_SOURCE="/usr/src/linux-headers-$(uname -r)"

# sign-file 유틸리티 경로 확인
SIGN_FILE="${KERNEL_SOURCE}/scripts/sign-file"

# 커널 모듈 서명
if [[ -x "${SIGN_FILE}" ]]; then
  echo "커널 모듈 서명 시작..."
  "${SIGN_FILE}" sha256 private.key public.der "${MODULE_NAME}"
  if [[ $? -eq 0 ]]; then
    echo "커널 모듈 서명 완료."
  else
    echo "커널 모듈 서명 실패!"
    exit 1
  fi
else
  echo "sign-file 유틸리티를 찾을 수 없습니다: ${SIGN_FILE}"
  exit 1
fi

echo "스크립트 실행 완료."
