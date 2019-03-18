/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>  //for getopt_long

#include <sys/ptrace.h>
#include <sys/user.h>   // for user
#include <sys/types.h> // for pid_t
#include <errno.h>     // for errno
#include <string.h>    // for strerror
#include <unistd.h>    // for getpid
#include <functional>  // for std::function
#include <sys/prctl.h>
#include <sys/wait.h>  // waitpid
#include <unistd.h>

const char* my_version = "1.0";

static void show_help(const char *cmd)
{
    fprintf(stderr,"Usage: %s [options]\n", cmd);
    fprintf(stderr, "options include:\n"
                    "  --version, -v        : show version\n"
                    "  --showint, -s value  : show an int value specified by user\n"
                    "  --runwptest, -r      : run watch point test\n");
}

// how to set watchpoint and breakpoint on different platform(arm/arm64/x86), pls ref:
// http://androidxref.com/9.0.0_r3/xref/bionic/tests/sys_ptrace_test.cpp
static void set_watchpoint(pid_t child, uintptr_t address, size_t size)
{
    //ASSERT_EQ(0u, address & 0x7) << "address: " << address;
    if ((address & 0x7) != 0u) {
        printf("ERR: address invalid: %lu\n", address);
        return;
    }

    if(ptrace(PTRACE_POKEUSER, child, offsetof(user, u_debugreg[0]), address) != 0) {
        printf("ERR : PTRACE_POKEUSER for address error : %s\n", strerror(errno));
        return;
    }

    errno = 0;
    unsigned data = ptrace(PTRACE_PEEKUSER, child, offsetof(user, u_debugreg[7]), nullptr);
    if (errno != 0) {
        printf("ERR : PTRACE_PEEKUSER for address error : %s\n", strerror(errno));
        return;
    }
    
    const unsigned size_flag = (size == 8) ? 2 : size - 1;
    const unsigned enable = 1;
    const unsigned type = 1; // Write.

    const unsigned mask = 3 << 18 | 3 << 16 | 1;
    const unsigned value = size_flag << 18 | type << 16 | enable;
    data &= mask;
    data |= value;
    if(ptrace(PTRACE_POKEUSER, child, offsetof(user, u_debugreg[7]), data) != 0) {
        printf("ERR : PTRACE_PEEKUSER for address error : %s\n", strerror(errno));
        return;
    }
}

template <typename T>
static void run_watchpoint_test(std::function<void(T&)> child_func, size_t offset, size_t size) {
  alignas(16) T data{};

  pid_t child = fork();
  if (child < 0) {
      printf("fork failed : %s\n", strerror(errno));
      return;
  }

  if (child == 0) {
    // Extra precaution: make sure we go away if anything happens to our parent.
    if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) == -1) {
      perror("prctl(PR_SET_PDEATHSIG)");
      _exit(1);
    }

    if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
      perror("ptrace(PTRACE_TRACEME)");
      _exit(2);
    }

    child_func(data);
    _exit(0);
  }

  int status;
  printf("wait for child send SIGSTOP\n");
  if (child != TEMP_FAILURE_RETRY(waitpid(child, &status, __WALL))) {
      printf("waitpid@1 failed : %s\n", strerror(errno));
      return;
  }
  if (!WIFSTOPPED(status)) {
      printf("Status@1 was: : %d\n", status);
  }
  if (SIGSTOP != WSTOPSIG(status)) {
      printf("Status@2 was: : %d\n", status);
  }
  printf("receive child sent SIGSTOP\n");

  printf("begin set watchpoint\n");
  set_watchpoint(child, uintptr_t(&data) + offset, size);
  printf("end set watchpoint\n");

  printf("wait for child send SIGTRAP\n");
  if (0 != ptrace(PTRACE_CONT, child, nullptr, nullptr)) {
      printf("PTRACE_CONT failed : %s\n", strerror(errno));
  }
  if (child != TEMP_FAILURE_RETRY(waitpid(child, &status, __WALL))) {
      printf("waitpid@2 failed : %s\n", strerror(errno));
  }
  if(!WIFSTOPPED(status)) {
      printf("Status@3 was: : %d\n", status);
  }
  if(SIGTRAP != WSTOPSIG(status)) {
      printf("Status@4 was: : %d\n", status);
  }
  printf("receive child sent SIGTRAP\n");
}

template <typename T>
static void watchpoint_child(T& data) {
  raise(SIGSTOP);  // Synchronize with the tracer, let it set the watchpoint.
  printf("begin trigger data\n");
  (void) data;
  data = 1;  // Now trigger the watchpoint.
  printf("end trigger data\n");
}

static void test_watchpoint()
{
    run_watchpoint_test<uint64_t>(std::bind(watchpoint_child<uint64_t>, std::placeholders::_1), 0, sizeof(uint64_t));
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        show_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    opterr = 0;
    do {
        int c;
        int option_index = 0;
        static const struct option long_options[] = {
          { "version",       no_argument,       NULL,   'v' },
          { "showint",       required_argument, NULL,   's' },
          { "runwptest",       no_argument, NULL,   'r' },
          { NULL,            0,                 NULL,   0 }
        };

        c = getopt_long(argc, argv, "vs:r", long_options, &option_index);

        if (c == EOF) {
            break;
        }

        switch (c) {
		case 'v':
            printf("version %s\n", my_version);
            break;
        case 's':
            printf("show value : %d\n", atoi(optarg));
            break;
        case 'r':
            test_watchpoint();
            break;
        default:
            show_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    } while (1);

    if(argc > optind + 1) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
