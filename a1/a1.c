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
#define VERSION_SIZE 2
#define NO_OF_SECTIONS_SIZE 1
#define SECT_NAME_SIZE 11
#define SECT_TYPE_SIZE 2
#define SECT_OFFSET_SIZE 4
#define SECT_SIZE_SIZE 4
#define SECTION_HEADERS_SIZE (SECT_NAME_SIZE + SECT_TYPE_SIZE + SECT_OFFSET_SIZE + SECT_SIZE_SIZE)

// good values
#define MAGIC "BTT1"
#define VERSION_MIN (short)68
#define VERSION_MAX (short)155

#define SECT_TYPE_70 0x46
#define SECT_TYPE_64 0x40
#define SECT_TYPE_77 0x4D
#define SECT_TYPE_93 0x5D

#define ACCEPTED_NO_OF_SECTIONS 2
#define NO_OF_SECTIONS_MIN 4
#define NO_OF_SECTIONS_MAX 16

#define MAX_PATH_LENGTH 1024

#define VARIANT 33968

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

        if (lstat(fullPath, &inode) != 0)
            continue;

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
    if (dir == NULL) // invalid path specified
    {
        printf("ERROR\ninvalid directory path\n");
        return;
    }
    closedir(dir);

    printf("SUCCESS\n");
    printDirectory(path, isRecursive, startsWith, hasPermExecute);
}

bool handleParse(int argc, char *argv[], bool okToPrint)
{
    char *path = NULL;
    for (int i = 2; i < argc; i++)
    {
        if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
    }

    if (path == NULL)
    {
        return false;
    }

    short int version = 0;
    char noOfSections = 0;
    char magic[MAGIC_SIZE];

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return false;
    }

    if (read(fd, magic, MAGIC_SIZE) < MAGIC_SIZE || strncmp(magic, MAGIC, MAGIC_SIZE) != 0)
    {
        if (okToPrint)
            printf("ERROR\nwrong magic\n");
        close(fd);
        return false;
    }

    lseek(fd, 2, SEEK_CUR); // skip the header
    if (read(fd, &version, VERSION_SIZE) < VERSION_SIZE || version < VERSION_MIN || version > VERSION_MAX)
    {
        if (okToPrint)
            printf("ERROR\nwrong version\n");
        close(fd);
        return false;
    }

    if (read(fd, &noOfSections, NO_OF_SECTIONS_SIZE) < NO_OF_SECTIONS_SIZE || (noOfSections != ACCEPTED_NO_OF_SECTIONS && (noOfSections < NO_OF_SECTIONS_MIN || noOfSections > NO_OF_SECTIONS_MAX)))
    {
        if (okToPrint)
            printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return false;
    }

    char result[5000] = "";

    strcat(result, "version=");

    char versionStr[10];
    snprintf(versionStr, sizeof(versionStr), "%d", version);
    strcat(result, versionStr);
    strcat(result, "\n");

    strcat(result, "nr_sections=");

    char noOfSectionsStr[10];
    snprintf(noOfSectionsStr, sizeof(noOfSectionsStr), "%d", noOfSections);
    strcat(result, noOfSectionsStr);
    strcat(result, "\n");

    bool success = true;

    for (int section = 0; section < noOfSections; section++)
    {
        char sectName[SECT_NAME_SIZE + 1];
        short int sectType;
        int sectOffset, sectSize;

        lseek(fd, MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE + NO_OF_SECTIONS_SIZE + section * SECTION_HEADERS_SIZE, SEEK_SET);
        read(fd, sectName, SECT_NAME_SIZE);
        sectName[SECT_NAME_SIZE] = '\0';
        read(fd, &sectType, SECT_TYPE_SIZE);
        read(fd, &sectOffset, SECT_OFFSET_SIZE);
        read(fd, &sectSize, SECT_SIZE_SIZE);

        if (sectType != SECT_TYPE_70 && sectType != SECT_TYPE_64 && sectType != SECT_TYPE_77 && sectType != SECT_TYPE_93)
        {
            if (okToPrint)
                printf("ERROR\nwrong sect_types\n");
            success = false;
            close(fd);
            return false;
        }
        else
        {
            strcat(result, "section");

            char sectNr[4];
            snprintf(sectNr, sizeof(sectNr), "%d", section + 1);
            strcat(result, sectNr);

            strcat(result, ": ");
            strcat(result, sectName);

            char sectInfo[50];
            snprintf(sectInfo, sizeof(sectInfo), " %d %d\n", sectType, sectSize);
            strcat(result, sectInfo);
        }
    }

    if (success)
    {
        if (okToPrint)
            printf("SUCCESS\n%s", result);
        close(fd);
        return true;
    }

    close(fd);
    return false;
}

void handleExtract(int argc, char *argv[])
{
    char *path = NULL;
    int sectionNr = 0;
    int lineNr = 0;

    for (int i = 2; i < argc; i++)
    {
        if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
        else if (strncmp(argv[i], "section=", 8) == 0)
            sectionNr = atoi(argv[i] + 8);
        else if (strncmp(argv[i], "line=", 5) == 0)
            lineNr = atoi(argv[i] + 5);
    }

    if (path == NULL)
    {
        printf("ERROR\ninvalid file\n");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf("ERROR\ninvalid file\n");
        return;
    }

    if (!handleParse(argc, argv, false))
    {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return;
    }

    char noOfSections = 0;

    lseek(fd, MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE, SEEK_SET);
    read(fd, &noOfSections, NO_OF_SECTIONS_SIZE);

    if (sectionNr < 1 || sectionNr > noOfSections)
    {
        printf("ERROR\ninvalid section\n");
        close(fd);
        return;
    }

    if (lineNr < 1)
    {
        printf("ERROR\ninvalid line\n");
        close(fd);
        return;
    }

    int sectOffset, sectSize;

    lseek(fd, MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE + NO_OF_SECTIONS_SIZE + (sectionNr - 1) * SECTION_HEADERS_SIZE + SECT_NAME_SIZE + SECT_TYPE_SIZE, SEEK_SET); // skip name and type

    // lseek(fd, SECT_NAME_SIZE + SECT_TYPE_SIZE , SEEK_CUR); // skip the name and type
    read(fd, &sectOffset, SECT_OFFSET_SIZE);
    read(fd, &sectSize, SECT_SIZE_SIZE);

    char *sectionContent = malloc(sectSize + 1);
    if (sectionContent == NULL)
    {
        close(fd);
        return;
    }

    lseek(fd, sectOffset, SEEK_SET);
    read(fd, sectionContent, sectSize);
    sectionContent[sectSize] = '\0';

    char *line = strtok(sectionContent, "\n");
    int currentLine = 1;
    bool foundLine = false;

    while (line != NULL)
    {
        if (currentLine == lineNr)
        {
            printf("SUCCESS\n%s\n", line);
            foundLine = true;
            break;
        }
        line = strtok(NULL, "\n");
        currentLine++;
    }

    if (!foundLine)
        printf("ERROR\ninvalid line\n");

    free(sectionContent);
    close(fd);
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
            handleParse(argc, argv, true);
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