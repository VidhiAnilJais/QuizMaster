# ---------------------------------------------------------------------------
#  QuizMaster - convenience Makefile
#
#  Linux / macOS :  make
#  Windows MinGW :  mingw32-make     (or simply run build.bat)
# ---------------------------------------------------------------------------

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
TARGET   ?= build/QuizMaster

# "mkdir -p" and "rm -rf" do not exist in the Windows shell, so pick per platform.
ifeq ($(OS),Windows_NT)
MKDIR  = mkdir
RM     = rmdir /s /q
EXE    = .exe
else
MKDIR  = mkdir -p
RM     = rm -rf
EXE    =
endif

SOURCES  := src/main.cpp \
            src/UI.cpp \
            src/FileStore.cpp \
            src/User.cpp \
            src/Student.cpp \
            src/Admin.cpp \
            src/Question.cpp \
            src/Quiz.cpp \
            src/Result.cpp

HEADERS  := include/User.h \
            include/Student.h \
            include/Admin.h \
            include/Question.h \
            include/Quiz.h \
            include/Result.h \
            include/UI.h \
            include/FileStore.h

.PHONY: all run clean test

all: $(TARGET)

# Order-only prerequisite: the recipe runs only when build/ is missing, so the
# build also works on Windows where "mkdir -p" fails if the folder exists.
$(TARGET): $(SOURCES) $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)
	@echo Build finished : $(TARGET)$(EXE)
	@echo Run it with    : $(TARGET)$(EXE)

build:
	@$(MKDIR) build

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	@build\QuizMaster.exe
else
	@./$(TARGET)
endif

clean:
	@$(RM) build
	@echo build folder removed

# Quick smoke test: the executable must start, parse its arguments and exit.
# The full test matrix is documented in docs/TEST_REPORT.md and can be run with
#   powershell -ExecutionPolicy Bypass -File docs\run-tests.ps1
test: $(TARGET)
ifeq ($(OS),Windows_NT)
	@build\QuizMaster.exe --help > nul && echo smoke test passed
else
	@./$(TARGET) --help > /dev/null && echo "smoke test passed"
endif