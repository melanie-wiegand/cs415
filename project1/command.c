#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include "command.h"

FILE *output = NULL;

int outwrite = STDOUT_FILENO;

void writeToOutput(char* msg)
{
    write(outwrite, msg, strlen(msg));
}

void listDir()
{
    /*for the ls command*/

    DIR *curdir;
    // current directory
    curdir = opendir(".");

    if (curdir == NULL) 
    {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry = readdir(curdir);
    while (entry != NULL)
    {
        size_t len = strlen(entry->d_name);
        write(outwrite, entry->d_name, len);
        write(outwrite, "\t", 1);
        entry = readdir(curdir);
    }
    write(outwrite, "\n", 1);

    closedir(curdir);
} 

void showCurrentDir()
{
    /*for the pwd command*/

    // char *msg = "Current working directory: ";

    char buffer[1000];
    if (getcwd(buffer, sizeof(buffer)) != NULL)
    {  
        // write(STDOUT_FILENO, msg, strlen(msg));
        write(outwrite, buffer, strlen(buffer));
        write(outwrite, "\n", 1);
    } else
    {
        perror("Error retrieving current path");
    }
} 

void makeDir(char *dirName)
{
    /*for the mkdir command*/

    // char *msg = "Directory created successfully\n";

    // standard perms
    if (mkdir(dirName, 0755) == 0)
    {
        // write(outwrite, msg, strlen(msg));
        ;
    } else
    {
        // perror("Could not create directory");
        writeToOutput("Could not create directory");
    }
} 

void changeDir(char *dirName)
{
    /*for the cd command*/
    // char *msg = "Changed to directory \"";
    if (chdir(dirName) == 0)
    {
        // write(outwrite, msg, strlen(msg));
        // write(outwrite, dirName, strlen(dirName));
        // write(outwrite, "\"\n", 2);
        ;
    } else
    {
        perror("Directory not found");
    }
} 

void copyFile(char *sourcePath, char *destinationPath)
{
    /*for the cp command*/
    int src = open(sourcePath, O_RDONLY);
    if (src == -1)
    {
        perror("Could not open source file");
        return;
    }

    int isdir = 0;
    int dir = open(destinationPath, O_RDONLY | O_DIRECTORY);

    if (dir != -1)
    {
        isdir = 1;
        close(dir);
    }

    int dst;
    
    if (isdir)
    {
        char path[1024];
        char* filename = basename(sourcePath);
        
        strcpy(path, destinationPath);
        if (destinationPath[strlen(destinationPath) - 1] != '/')
        {
            strcat(path, "/");
        }
        strcat(path, filename);

        dst = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    }
    else
    {
        dst = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    }

    if (dst == -1)
    {
        perror("Could not open/create destination file");
        close(src);
        return;
    }
    

    char buffer[100000];
    ssize_t textsize = read(src, buffer, 100000);

    if (textsize == -1)
    {
        perror("Error reading from source file");
        close(src);
        return;
    }

    while (textsize > 0)
    {
        ssize_t checksize = write(dst, buffer, textsize);
        if (checksize != textsize)
        {
            perror("Error writing to destination file"); 
            close(src);
            close(dst);
            return;
        }
        textsize = read(src, buffer, 100000);
    }

    close(src);
    close(dst);
} 

void moveFile(char *sourcePath, char *destinationPath)
{
    /*for the mv command*/
    // char *msg = "File moved successfully\n";

    int isdir = 0;
    int dir = open(destinationPath, O_RDONLY | O_DIRECTORY);

    if (dir != -1)
    {
        isdir = 1;
        close(dir);
    }

    char path[1024];

    if (isdir)
    {
        char* filename = basename(sourcePath);
        
        strcpy(path, destinationPath);
        if (destinationPath[strlen(destinationPath) - 1] != '/')
        {
            strcat(path, "/");
        }
        strcat(path, filename);
    }
    else
    {
        strcpy(path, destinationPath);
    }

    if (rename(sourcePath, path) == 0)
    {
        // write(outwrite, msg, strlen(msg));
        ;
    } 
    else
    {
        perror("Could not move/rename file");
    }
} 

void deleteFile(char *filename)
{
    /*for the rm command*/
    // char *msg = "Removed file \"";

    // standard perms
    if (unlink(filename) == 0)
    {
        // write(outwrite, msg, strlen(msg));
        // write(outwrite, filename, strlen(filename));
        // write(outwrite, "\"\n", 2);
        ;
    } else
    {
        perror("Could not remove file");
    }
} 

void displayFile(char *filename)
{
    /*for the cat command*/
    int src = open(filename, O_RDONLY);
    if (src == -1)
    {
        perror("Could not open source file");
        return;
    }

    char buffer[100000];
    ssize_t textsize = read(src, buffer, 100000);

    if (textsize == -1)
    {
        perror("Error reading from source file");
        close(src);
        return;
    }

    while (textsize > 0)
    {
        ssize_t checksize = write(outwrite, buffer, textsize);
        if (checksize != textsize)
        {
            perror("Error writing to terminal"); 
            close(src);
            return;
        }
        textsize = read(src, buffer, 100000);
    }
    // write(STDOUT_FILENO, "\n", 1);

    close(src);
} 

