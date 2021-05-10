#define _POSIX_C_SOURCE 200112L

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <args.h>
#include <data-structures.h>

extern bool helpFlag;
extern bool logFlag;
extern string* socketFileName;

int main(int argc, char *argv[]) {

    // vector_string args[10];
    // for(int i=0; i<10; i++) args[i] = *new_vector_string();

    parseCommandLineArguments(argc, argv);

    if(socketFileName != NULL) {
        printf("%s\n", socketFileName->content);
    } else {
        printf("socketpath is null\n");
    }

    
    if(socketFileName != NULL) {
        socketFileName->free(socketFileName);
    }
    
    // printf("tokenization done\n");

    // for(int i=0; i<args[0].size; i++) {
    //     printf("%s\n", args[0].array[i].array);
    // }

    // vector_string* v[10];
    // for(int i=0; i<10; i++) v[i] = new_vector_string();
    // for(int i=0; i<50; i++) {
    //     int j = rand() % 10;
    //     v[j]->append(v[j], new_string("ciao"));
    // }
    // for(int i=0; i<10; i++) v[0]->append(v[0], new_string("ciao"));
    // for(int i=0; i<10; i++) printf("V[%d], size:%d, cap:%d\n", i, v[i]->size, v[i]->capacity);
    // for(int i=0; i<10; i++) v[i]->free(v[i]);

    // for(int i=0; i<10; i++) args[i].free(&args[i]);

    return 0;
}

// typedef int (*argfuncs)(const char *const);

// int mapfunc(char c)
// {
//     switch (c)
//     {
//     case 'n':
//         return 0;
//     case 'm':
//         return 1;
//     case 'o':
//         return 2;
//     case 'h':
//         return 3;
//     }
//     return -1;
// }

// int main(int argc, char *argv[])
// {
//     argfuncs V[] = {printn, printm, printo, printh};

//     int opt;
//     while ((opt = getopt(argc, argv, "n:m:o:h")) != -1)
//     {
//         switch (opt)
//         {
//         case '?':
//             break;
//         case 'h':
//         {
//             V[mapfunc(opt)](argv[0]);
//             break;
//         }
//         default:
//         {
//             V[mapfunc(opt)](optarg);
//         }
//         }
//     }
//     return 0;
// }