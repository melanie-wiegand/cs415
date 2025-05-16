#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>    // for getcwd()
#include <sys/stat.h>  // for stat()
#include <ctype.h>

void lfcat() {
    DIR *dir;
    struct dirent *entry;
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    dir = opendir(cwd);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Optional: skip files that aren't .txt or similar (like output.txt, main.c, etc.)
        if (strstr(entry->d_name, ".c") || strstr(entry->d_name, ".exe"))
            continue;

        FILE *fp = fopen(entry->d_name, "r");
        if (!fp) {
            // skip unreadable files
            continue;
        }

        printf("\n=== %s ===\n", entry->d_name);

        char *line = NULL;
        size_t len = 0;

        while (getline(&line, &len, fp) != -1) {
            printf("%s", line);
        }

        free(line);
        fclose(fp);

        // print 80 dashes
        printf("\n%.*s\n", 80, "--------------------------------------------------------------------------------");
    }

    closedir(dir);
}