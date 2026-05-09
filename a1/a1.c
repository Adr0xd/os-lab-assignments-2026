#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

#define ACCEPTED_NO_OF_SECTIONS 2
#define NO_OF_SECTIONS_MIN 4
#define NO_OF_SECTIONS_MAX 16
#define REQUIRED_NO_OF_SECTIONS_93 2

#define MAX_PATH_LENGTH 4096

#define VARIANT 33968

void print_variant() { printf("%d\n", VARIANT); }

void print_directory(char *path, bool recursive, char *starts_with,
                     bool has_perm_execute)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
        return;

    struct dirent *entry;
    struct stat inode;

    while ((entry = readdir(dir)) != NULL)
    {

        bool ok_to_print = true;

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &inode) != 0)
            continue;

        // ignore . and .. directories
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        // dont print the file/dir if it does not start with startsWith
        if (starts_with != NULL)
            if (strncmp(entry->d_name, starts_with, strlen(starts_with)) != 0)
                ok_to_print = false;

        // check if has_perm_exec
        if (has_perm_execute && !(inode.st_mode & S_IXUSR))
            ok_to_print = false;

        if (ok_to_print)
            printf("%s\n", full_path);

        if (recursive && S_ISDIR(inode.st_mode))
            print_directory(full_path, recursive, starts_with,
                            has_perm_execute);
    }

    closedir(dir);
}

void handle_list(int argc, char *argv[])
{
    bool is_recursive = false;
    bool has_perm_execute = false;
    char *path = NULL;
    char *starts_with = NULL;

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "recursive") == 0)
            is_recursive = true;
        else if (strcmp(argv[i], "has_perm_execute") == 0)
            has_perm_execute = true;
        else if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
        else if (strncmp(argv[i], "name_starts_with=", 17) == 0)
            starts_with = argv[i] + 17;
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
    print_directory(path, is_recursive, starts_with, has_perm_execute);
}

bool handle_parse(int argc, char *argv[], bool ok_to_print)
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
    char no_of_sections = 0;
    char magic[MAGIC_SIZE];

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return false;
    }

    if (read(fd, magic, MAGIC_SIZE) < MAGIC_SIZE ||
        strncmp(magic, MAGIC, MAGIC_SIZE) != 0)
    {
        if (ok_to_print)
            printf("ERROR\nwrong magic\n");
        close(fd);
        return false;
    }

    lseek(fd, 2, SEEK_CUR); // skip the header
    if (read(fd, &version, VERSION_SIZE) < VERSION_SIZE ||
        version < VERSION_MIN || version > VERSION_MAX)
    {
        if (ok_to_print)
            printf("ERROR\nwrong version\n");
        close(fd);
        return false;
    }

    if (read(fd, &no_of_sections, NO_OF_SECTIONS_SIZE) <
            NO_OF_SECTIONS_SIZE ||
        (no_of_sections != ACCEPTED_NO_OF_SECTIONS &&
         (no_of_sections < NO_OF_SECTIONS_MIN ||
          no_of_sections > NO_OF_SECTIONS_MAX)))
    {
        if (ok_to_print)
            printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return false;
    }

    char result[5000] = "";

    strcat(result, "version=");

    char version_str[10];
    snprintf(version_str, sizeof(version_str), "%d", version);
    strcat(result, version_str);
    strcat(result, "\n");

    strcat(result, "nr_sections=");

    char no_of_sections_str[10];
    snprintf(no_of_sections_str, sizeof(no_of_sections_str), "%d",
             no_of_sections);
    strcat(result, no_of_sections_str);
    strcat(result, "\n");

    bool success = true;

    for (int section = 0; section < no_of_sections; section++)
    {
        char section_name[SECT_NAME_SIZE + 1];
        short int section_type;
        int section_offset, section_size;

        lseek(fd,
              MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE + NO_OF_SECTIONS_SIZE +
                  section * SECTION_HEADERS_SIZE,
              SEEK_SET);
        read(fd, section_name, SECT_NAME_SIZE);
        section_name[SECT_NAME_SIZE] = '\0';
        read(fd, &section_type, SECT_TYPE_SIZE);
        read(fd, &section_offset, SECT_OFFSET_SIZE);
        read(fd, &section_size, SECT_SIZE_SIZE);

        if (section_type != SECT_TYPE_70 && section_type != SECT_TYPE_64 &&
            section_type != SECT_TYPE_77 && section_type != SECT_TYPE_93)
        {
            if (ok_to_print)
                printf("ERROR\nwrong sect_types\n");
            success = false;
            close(fd);
            return false;
        }
        else
        {
            strcat(result, "section");

            char section_number[4];
            snprintf(section_number, sizeof(section_number), "%d",
                     section + 1);
            strcat(result, section_number);

            strcat(result, ": ");
            strcat(result, section_name);

            char section_info[50];
            snprintf(section_info, sizeof(section_info), " %d %d\n",
                     section_type, section_size);
            strcat(result, section_info);
        }
    }

    if (success)
    {
        if (ok_to_print)
            printf("SUCCESS\n%s", result);
        close(fd);
        return true;
    }

    close(fd);
    return false;
}

void handle_extract(int argc, char *argv[])
{
    char *path = NULL;
    int section_number = 0;
    int line_number = 0;

    for (int i = 2; i < argc; i++)
    {
        if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
        else if (strncmp(argv[i], "section=", 8) == 0)
            section_number = atoi(argv[i] + 8);
        else if (strncmp(argv[i], "line=", 5) == 0)
            line_number = atoi(argv[i] + 5);
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

    if (!handle_parse(argc, argv, false))
    {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return;
    }

    char no_of_sections = 0;

    lseek(fd, MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE, SEEK_SET);
    read(fd, &no_of_sections, NO_OF_SECTIONS_SIZE);

    if (section_number < 1 || section_number > no_of_sections)
    {
        printf("ERROR\ninvalid section\n");
        close(fd);
        return;
    }

    if (line_number < 1)
    {
        printf("ERROR\ninvalid line\n");
        close(fd);
        return;
    }

    int section_offset, section_size;

    lseek(fd,
          MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE + NO_OF_SECTIONS_SIZE +
              (section_number - 1) * SECTION_HEADERS_SIZE + SECT_NAME_SIZE +
              SECT_TYPE_SIZE,
          SEEK_SET); // skip name and type

    // lseek(fd, SECT_NAME_SIZE + SECT_TYPE_SIZE , SEEK_CUR); // skip the name
    // and type
    read(fd, &section_offset, SECT_OFFSET_SIZE);
    read(fd, &section_size, SECT_SIZE_SIZE);

    char *section_content = malloc(section_size + 1);
    if (section_content == NULL)
    {
        close(fd);
        return;
    }

    lseek(fd, section_offset, SEEK_SET);
    read(fd, section_content, section_size);
    section_content[section_size] = '\0';

    char *line = strtok(section_content, "\n");
    int current_line = 1;
    bool found_line = false;

    while (line != NULL)
    {
        if (current_line == line_number)
        {
            printf("SUCCESS\n%s\n", line);
            found_line = true;
            break;
        }
        line = strtok(NULL, "\n");
        current_line++;
    }

    if (!found_line)
        printf("ERROR\ninvalid line\n");

    free(section_content);
    close(fd);
    return false;
}

void print_directory_2(char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
        return;

    struct dirent *entry;
    struct stat inode;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", path, entry->d_name);

        if (lstat(full_path, &inode) != 0)
            continue;

        if (S_ISREG(inode.st_mode))
        {
            int fd = open(full_path, O_RDONLY);
            if (fd < 0)
                continue;

            char magic[MAGIC_SIZE];
            short version;
            char no_of_sections;

            if (read(fd, magic, MAGIC_SIZE) < MAGIC_SIZE ||
                strncmp(magic, MAGIC, MAGIC_SIZE) != 0)
            {
                close(fd);
                continue;
            }

            lseek(fd, 2, SEEK_CUR);

            if (read(fd, &version, VERSION_SIZE) < VERSION_SIZE ||
                version < VERSION_MIN || version > VERSION_MAX)
            {
                close(fd);
                continue;
            }

            if (read(fd, &no_of_sections, NO_OF_SECTIONS_SIZE) <
                    NO_OF_SECTIONS_SIZE ||
                (no_of_sections != ACCEPTED_NO_OF_SECTIONS &&
                 (no_of_sections < NO_OF_SECTIONS_MIN ||
                  no_of_sections > NO_OF_SECTIONS_MAX)))
            {
                close(fd);
                continue;
            }

            int count = 0;

            for (int section = 0; section < no_of_sections; section++)
            {
                short section_type;

                lseek(fd,
                      MAGIC_SIZE + HEADER_SIZE + VERSION_SIZE +
                          NO_OF_SECTIONS_SIZE +
                          section * SECTION_HEADERS_SIZE + SECT_NAME_SIZE,
                      SEEK_SET);

                read(fd, &section_type, SECT_TYPE_SIZE);

                if (section_type == SECT_TYPE_93)
                    count++;

                if (section_type != SECT_TYPE_70 &&
                    section_type != SECT_TYPE_64 &&
                    section_type != SECT_TYPE_77 &&
                    section_type != SECT_TYPE_93)
                {
                    count = 0;
                    break;
                }
            }

            if (count >= REQUIRED_NO_OF_SECTIONS_93)
                printf("%s\n", full_path);

            close(fd);
        }

        if (S_ISDIR(inode.st_mode))
            print_directory_2(full_path);
    }

    closedir(dir);
}

void handle_findall(int argc, char *argv[])
{
    char *path = NULL;

    for (int i = 2; i < argc; i++)
    {
        if (strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
    }

    if (path == NULL)
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
    print_directory_2(path);
}

int main(int argc, char *argv[])
{
    // list, parse, extract, findall
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            print_variant();
        }
        else if (strcmp(argv[1], "list") == 0)
        {
            handle_list(argc, argv);
        }
        else if (strcmp(argv[1], "parse") == 0)
        {
            handle_parse(argc, argv, true);
        }
        else if (strcmp(argv[1], "extract") == 0)
        {
            handle_extract(argc, argv);
        }
        else if (strcmp(argv[1], "findall") == 0)
        {
            handle_findall(argc, argv);
        }
        else
        {
            printf("Invalid command\n");
        }
    }

    return 0;
}