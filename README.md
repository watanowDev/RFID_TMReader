# RFID Wrapper Project

## 개요
본 프로젝트는 **ThingMagic MercuryAPI C API**를 기반으로 Linux 환경에서 사용할 수 있는 래퍼(wrapper) 라이브러리와 테스트 프로그램을 제공합니다.

구성 요소:

- C99 기반 **C 래퍼 라이브러리** (공유 라이브러리, `.so`)
- C99 + C++17 기반 **C++ 래퍼 라이브러리** (공유 라이브러리, `.so`)
- C 래퍼 테스트 프로그램
- C++ 래퍼 테스트 프로그램

원본 MercuryAPI SDK는 `third_party` 경로에 그대로 유지되며, 프로젝트 코드는 이를 직접 수정하지 않고 래핑하는 구조로 설계되었습니다.

---

## 개발 환경

- OS: Ubuntu 24.04
- IDE: JetBrains CLion
- Build System: CMake
- Language Standard
    - C Wrapper: C99
    - C++ Wrapper: C++17

---
## 프로젝트 구조

```
.
├── c_lib
│   └── api
├── cpp_lib
│   └── api
├── cpp_test
│   ├── config
│   └── src
├── c_test
│   └── src
├── install
│   ├── debug
│   │   ├── include
│   │   │   └── rfid
│   │   │       ├── mercuryapi
│   │   │       └── mercuryapi_cpp
│   │   └── lib
│   │       └── rfid
│   │           ├── mercuryapi
│   │           └── mercuryapi_cpp
│   └── release
│       ├── include
│       │   └── rfid
│       │       ├── mercuryapi
│       │       └── mercuryapi_cpp
│       └── lib
│           └── rfid
│               ├── mercuryapi
│               └── mercuryapi_cpp
└── third_party
    └── mercuryapi
        └── c
            ├── include
            └── src
├── CMakeLists.txt
└── README.md
```

### 디렉터리 설명

| 디렉터리                     | 설명                                                                                                     |
| ------------------------ | ------------------------------------------------------------------------------------------------------ |
| c_lib                    | C99 기반 C 래퍼 라이브러리 (.so) 생성용 CMake 프로젝트. `libmercuryapi.so` / `libmercuryapi_d.so` 생성                   |
| cpp_lib                  | C99 + C++17 기반 C++ 래퍼 라이브러리 (.so) 생성용 CMake 프로젝트. `libmercuryapi_cpp.so` / `libmercuryapi_cpp_d.so` 생성 |
| c_test                   | `c_lib` 결과물 테스트 프로젝트                                                                                   |
| cpp_test                 | `cpp_lib` 결과물 테스트 프로젝트                                                                                 |
| c_lib/api                | C 래퍼 인터페이스 파일 (`rfid_api.h`, `rfid_types.h`, `rfid_api.c`)                                             |
| cpp_lib/api              | C++ 래퍼 클래스 파일 (`mercuryapi.hpp`, `mercuryapi.cpp`)                                                     |
| cpp_test/config          | C++ 테스트 프로젝트 설정 파일                                                                                     |
| cpp_test/src             | C++ 테스트 코드                                                                                             |
| c_test/src               | C 테스트 코드                                                                                               |
| install/debug            | 디버깅용 빌드 결과물. `include`와 `lib` 하위에 각각 `rfid/mercuryapi`와 `rfid/mercuryapi_cpp` 결과물이 존재                  |
| install/release          | 배포용 빌드 결과물. 구조는 debug와 동일하지만 최적화된 릴리스 빌드                                                               |
| third_party/mercuryapi/c | ThingMagic MercuryAPI C SDK 원본 코드 (`include`와 `src`)                                                   |

### 비고

* `c_lib`는 순수 C99만 사용합니다.
* `cpp_lib`는 C++17 + C99를 사용합니다.
* `install/debug`와 `install/release`는 디버깅용과 배포용 라이브러리를 구분하여 관리합니다.
* `install/debug(or release)/rfid/mercuryapi`는 `c_lib` 결과물이 이관되며, `install/debug(or release)/rfid/mercuryapi_cpp`는 `cpp_lib` 결과물이 이관됩니다.

---
## 빌드 구조 (CLion 전용)

이 프로젝트는 **CLion에서 빌드와 install 경로 설정만으로** 빌드가 완료되도록 구성되어 있습니다.

### 특징

* `install` 폴더는 빌드 시 **자동 생성**됩니다.
* Build 경로와 install 경로를 지정하려면 하기의 방법을 따릅니다.

#### Build 경로 설정

1. 동일하게 CLion 설정 열기 (단축키: `Ctrl+Alt+S`)
2. **빌드, 실행, 배포** 탭 선택 → **CMake** 탭 선택
3. 하단 위치한 **[빌드 디렉터리]** 입력

```
build/debug  # 또는 release
```

#### Install 경로 설정

1. CLion 설정 열기 (단축키: `Ctrl+Alt+S`)
2. **빌드, 실행, 배포** 탭 선택 → **CMake** 탭 선택
3. 상단 위치한 **[CMake 옵션] Edit**란에 아래 입력

```
-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_SOURCE_DIR}/install/debug  # 또는 release
```

---

## Install 결과물

| 디렉터리              | 설명 |
|-------------------|------|
| `install/debug`   | Debug 빌드 결과물 |
| `install/release` | Release 빌드 결과물 |

해당 디렉터리에는 다음 항목들이 포함됩니다.

- 공유 라이브러리 (`.so`)
- 헤더 파일 (`.h`)

---

## 테스트 실행

테스트 프로그램은 **CLion의 Run / Debug 기능**을 사용하여 실행합니다.

테스트 대상 프로젝트:

- `c_test`
- `cpp_test`

---

## 참고 사항

- MercuryAPI 원본 SDK는 Window 의존성을 직접 수정했습니다.
- 모든 래퍼 로직은 third_party 외부에서 관리됩니다.
- 프로젝트는 유지보수성과 확장성을 고려하여 설계되었습니다.
