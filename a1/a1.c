#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

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

#define MAX_PATH_LENGTH 1024

#define VARIANT 33968

int readSectionFile(char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        return -1;
    }
    close(fd);
    return fd;
}

void printVariant()
{
    printf("%d\n", VARIANT);
}

void printDirectory(char *path, bool recursive, char *startsWith, bool hasPermExecute)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
        return;

    struct dirent *entry;
    struct stat inode;

    while ((entry = readdir(dir)) != NULL)
    {

        bool okToPrint = true;

        // check if has_perm_exec
        char fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        lstat(fullPath, &inode);

        // ignore . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // dont print the file/dir if it does not start with startsWith
        if (startsWith != NULL)
            if (strncmp(entry->d_name, startsWith, strlen(startsWith)) != 0)
                okToPrint = false;

        // check if has_perm_exec
        if (hasPermExecute && !(inode.st_mode & S_IXUSR))
            okToPrint = false;

        if (okToPrint)
            printf("%s\n", fullPath);

        if (recursive && S_ISDIR(inode.st_mode))
            printDirectory(fullPath, recursive, startsWith, hasPermExecute);
    }

    closedir(dir);
}

void handleList(int argc, char *argv[])
{
    bool isRecursive = false;
    bool hasPermExecute = false;
    char *path = NULL;
    char *startsWith = NULL;

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "recursive") == 0)
            isRecursive = true;
        else if (strcmp(argv[i], "has_perm_execute") == 0)
            hasPermExecute = true;
        else if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
        else if (strncmp(argv[i], "name_starts_with=", 17) == 0)
            startsWith = argv[i] + 17;
    }

    if (path == NULL) // no path specified
    {
        printf("ERROR\ninvalid directory path\n");
        return;
    }

    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        printf("ERROR\ninvalid directory path\n");
        return;
    }
    closedir(dir);

    printf("SUCCESS\n");

    printDirectory(path, isRecursive, startsWith, hasPermExecute);
}

void handleParse(int argc, char *argv[])
{
    printf("parse\n");
}

void handleExtract(int argc, char *argv[])
{
    printf("extract\n");
}

void handleFindAll(int argc, char *argv[])
{
    printf("findall\n");
}

int main(int argc, char *argv[])
{

    // list, parse, extract, findall
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printVariant();
        }
        else if (strcmp(argv[1], "list") == 0)
        {
            handleList(argc, argv);
        }
        else if (strcmp(argv[1], "parse") == 0)
        {
            handleParse(argc, argv);
        }
        else if (strcmp(argv[1], "extract") == 0)
        {
            handleExtract(argc, argv);
        }
        else if (strcmp(argv[1], "findall") == 0)
        {
            handleFindAll(argc, argv);
        }
        else
        {
            printf("Invalid command\n");
        }
    }

    return 0;
}