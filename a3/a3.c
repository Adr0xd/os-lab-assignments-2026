#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define uint unsigned int

// file format (bytes)
#define MAGIC_SIZE 4
#define HEADER_SIZE 2
#define VERSION_SIZE 2
#define NO_OF_SECTIONS_SIZE 1
#define SECT_NAME_SIZE 11
#define SECT_TYPE_SIZE 2
#define SECT_OFFSET_SIZE 4
#define SECT_SIZE_SIZE 4
#define SECTION_HEADERS_SIZE                                                 \
    (SECT_NAME_SIZE + SECT_TYPE_SIZE + SECT_OFFSET_SIZE + SECT_SIZE_SIZE)

// good values
#define MAGIC "BTT1"
#define VERSION_MIN (short)68
#define VERSION_MAX (short)155

#define SECT_TYPE_70 0x46
#define SECT_TYPE_64 0x40
#define SECT_TYPE_77 0x4D
#define SECT_TYPE_93 0x5D

#define END_CHAR 0x24

#define ACCEPTED_NO_OF_SECTIONS 2
#define NO_OF_SECTIONS_MIN 4
#define NO_OF_SECTIONS_MAX 16
#define REQUIRED_NO_OF_SECTIONS_93 2

#define MAX_PATH_LENGTH 4096

#define RESP_PIPE "RESP_PIPE_33968"
#define REQ_PIPE "REQ_PIPE_33968"
#define VARIANT 33968

#define DOLLAR_CH '$'
#define DOLLAR_STR "S"

#define SHM_NAME "/JHPUqf"
#define SHM_SIZE 3703784

int shm_fd = -1;
unsigned char *shm_ptr = NULL;
uint shm_size = 0;

int file_fd = -1;
unsigned char *file_ptr = NULL;
uint file_size = 0;

void write_string(int fd, char *str)
{
    write(fd, str, strlen(str));
    write(fd, "$", 1);
}

int read_string(int fd, char *buf)
{
    char c;
    int n = 0;

    while (read(fd, &c, 1) == 1)
    {
        if (c == DOLLAR_CH)
        {
            buf[n] = '\0';
            return 1;
        }

        buf[n++] = c;
    }

    buf[n] = '\0';
    return 0;
}

void write_number(int fd, uint number) { write(fd, &number, sizeof(uint)); }

uint read_number(int fd)
{
    uint number;
    read(fd, &number, sizeof(uint));
    return number;
}

int create_shm(uint size)
{
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0664);

    if (shm_fd == -1)
    {
        return 0;
    }

    if (ftruncate(shm_fd, size) == -1)
    {
        close(shm_fd);
        shm_fd = -1;
        return 0;
    }

    shm_ptr = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, shm_fd, 0);

    if (shm_ptr == MAP_FAILED)
    {
        close(shm_fd);
        shm_fd = -1;
        shm_ptr = NULL;
        return 0;
    }

    shm_size = size;
    return 1;
}

int map_file(char *filename)
{

    if (file_ptr != NULL)
    {
        munmap(file_ptr, file_size);
        file_ptr = NULL;
        file_size = 0;
    }

    if (file_fd != -1)
    {
        close(file_fd);
        file_fd = -1;
    }

    file_fd = open(filename, O_RDONLY);
    if (file_fd == -1)
        return 0;

    off_t size = lseek(file_fd, 0, SEEK_END);
    if (size <= 0)
    {
        close(file_fd);
        file_fd = -1;
        return 0;
    }

    lseek(file_fd, 0, SEEK_SET);

    file_ptr =
        (unsigned char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_fd, 0);

    if (file_ptr == MAP_FAILED)
    {
        close(file_fd);
        file_fd = -1;
        file_ptr = NULL;
        return 0;
    }

    file_size = (uint)size;

    close(file_fd);
    file_fd = -1;

    return 1;
}

int main()
{

    unlink(RESP_PIPE);

    if (mkfifo(RESP_PIPE, 0666) == -1)
    {
        printf("ERROR\n");
        printf("cannot create the response pipe\n");
        return 1;
    }

    int fd_request = open(REQ_PIPE, O_RDONLY);
    if (fd_request == -1)
    {
        printf("ERROR\n");
        printf("cannot open the request pipe\n");
        unlink(RESP_PIPE);
        return 1;
    }

    int fd_response = open(RESP_PIPE, O_WRONLY);
    if (fd_response == -1)
    {
        printf("ERROR\n");
        printf("cannot open the response pipe\n");
        close(fd_request);
        unlink(RESP_PIPE);
        return 1;
    }

    write_string(fd_response, "HELLO");

    printf("SUCCESS\n");

    char request[256];

    while (1)
    {
        if (!read_string(fd_request, request))
            break;

        if (strcmp(request, "PING") == 0)
        {
            write_string(fd_response, "PING");
            write_string(fd_response, "PONG");
            write_number(fd_response, VARIANT);
        }
        else if (strcmp(request, "EXIT") == 0)
            break;
        else if (strcmp(request, "CREATE_SHM") == 0)
        {
            uint shm_size = read_number(fd_request);

            write_string(fd_response, "CREATE_SHM");

            if (create_shm(shm_size))
                write_string(fd_response, "SUCCESS");
            else
                write_string(fd_response, "ERROR");
        }
        else if (strcmp(request, "WRITE_TO_SHM") == 0)
        {
            uint offset = read_number(fd_request);
            uint value = read_number(fd_request);
            write_string(fd_response, "WRITE_TO_SHM");

            if (shm_ptr != NULL && offset <= shm_size - sizeof(uint))
            {
                memcpy(shm_ptr + offset, &value, sizeof(uint));
                write_string(fd_response, "SUCCESS");
            }
            else
                write_string(fd_response, "ERROR");
        }
        else if (strcmp(request, "MAP_FILE") == 0)
        {
            char file_name[MAX_PATH_LENGTH];

            read_string(fd_request, file_name);

            write_string(fd_response, "MAP_FILE");

            if (map_file(file_name))
                write_string(fd_response, "SUCCESS");
            else
                write_string(fd_response, "ERROR");
        }
        else if (strcmp(request, "READ_FROM_FILE_OFFSET") == 0)
        {
            uint offset = read_number(fd_request);
            uint no_of_bytes = read_number(fd_request);

            write_string(fd_response, "READ_FROM_FILE_OFFSET");

            if (shm_ptr != NULL && file_ptr != NULL &&
                no_of_bytes <= shm_size && offset <= file_size &&
                no_of_bytes <= file_size - offset)
            {
                memcpy(shm_ptr, file_ptr + offset, no_of_bytes);
                write_string(fd_response, "SUCCESS");
            }
            else
                write_string(fd_response, "ERROR");
        }
    }

    close(fd_request);
    close(fd_response);
    unlink(RESP_PIPE);

    if (shm_ptr != NULL)
    {
        munmap(shm_ptr, shm_size);
        shm_ptr = NULL;
    }

    if (shm_fd != -1)
    {
        close(shm_fd);
        shm_fd = -1;
    }

    shm_unlink(SHM_NAME);

    return 0;
}