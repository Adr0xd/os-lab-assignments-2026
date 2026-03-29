#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// file format (bytes)
#define MAGIC_SIZE 4
#define HEADER_SIZE 2
#define VERSION 2
#define NO_OF_SECTIONS 1
#define SECTION_HEADERS 23
#define SECT_NAME 11
#define SECT_TYPE 2
#define SECT_OFFSET 4
#define SECT_SIZE 4

// good values
#define MAGIC "BTT1"
#define VERSION_MIN 68
#define VERSION_MAX 155

#define ACCEPTED_NO_OF_SECTIONS 2
#define NO_OF_SECTIONS_MIN 4
#define NO_OF_SECTIONS_MAX 16

#define VARIANT 33968


int main(int argc, char *argv[])
{

    if(argc == 2 && strcmp(argv[1], "variant") == 0) {
        printf("%d\n", VARIANT);
    }

    return 0;

}