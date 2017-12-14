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

const char* my_version = "1.0";

int main(int argc, char *argv[])
{
    opterr = 0;
    do {
        int c;
        int option_index = 0;
        static const struct option long_options[] = {
          { "version",       no_argument,       NULL,   'v' },
          { "showint",       required_argument, NULL,   's' },
          { NULL,            0,                 NULL,   0 }
        };

        c = getopt_long(argc, argv, "vs:", long_options, &option_index);

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
        case '?':
            fprintf(stderr, "usage: %s -v [-s intValue]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } while (1);

    if(argc > optind + 1) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
