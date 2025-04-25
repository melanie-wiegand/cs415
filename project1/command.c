#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>


#include "command.h"


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
        writeToOutput("Error opening directory\n");
        return;
    }

    struct dirent *entry = readdir(curdir);
    while (entry != NULL)
    {
        writeToOutput(entry->d_name);
        write(outwrite, " ", 1);
        entry = readdir(curdir);
    }
    write(outwrite, "\n", 1);

    closedir(curdir);
} 

void showCurrentDir()
{
    /*for the pwd command*/

    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL)
    {  
        writeToOutput(buffer);
        write(outwrite, "\n", 1);
    } else
    {
        writeToOutput("Error retrieving current path\n");
    }
} 

void makeDir(char *dirName)
{
    /*for the mkdir command*/

    // standard perms
    if (mkdir(dirName, 0755) != 0)
    {
        writeToOutput("Could not create directory\n");
    }
} 

void changeDir(char *dirName)
{
    /*for the cd command*/

    if (chdir(dirName) != 0)
    {
        writeToOutput("Directory not found\n");
    }
} 

void copyFile(char *sourcePath, char *destinationPath)
{
    /*for the cp command*/

    int src = open(sourcePath, O_RDONLY);
    if (src == -1)
    {
        writeToOutput("Could not open source file\n");
        return;
    }

    // handle directory destination
    int isdir = 0;
    int dir = open(destinationPath, O_RDONLY | O_DIRECTORY);

    if (dir != -1)
    {
        isdir = 1;
        close(dir);
    }

    int dst;
    
    // destination is a directory
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

    // destination is a file
    else
    {
        dst = open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    }

    if (dst == -1)
    {
        writeToOutput("Could not open/create destination file\n");
        close(src);
        return;
    }
    

    char buffer[100000];
    ssize_t textsize = read(src, buffer, 100000);

    if (textsize == -1)
    {
        writeToOutput("Error reading from source file\n");
        close(src);
        return;
    }

    while (textsize > 0)
    {
        ssize_t checksize = write(dst, buffer, textsize);
        if (checksize != textsize)
        {
            writeToOutput("Error writing to destination file\n"); 
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

    int isdir = 0;
    int dir = open(destinationPath, O_RDONLY | O_DIRECTORY);

    if (dir != -1)
    {
        isdir = 1;
        close(dir);
    }

    char path[1024];

    // move to directory
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

    // move (rename) to file
    if (rename(sourcePath, path) != 0)
    {
        writeToOutput("Could not move/rename file\n");
    }
} 

void deleteFile(char *filename)
{
    /*for the rm command*/

    if (unlink(filename) != 0)
    {
        writeToOutput("Could not remove file\n");
    }
} 

void displayFile(char *filename)
{
    /*for the cat command*/
    
    int src = open(filename, O_RDONLY);
    if (src == -1)
    {
        writeToOutput("Could not open source file\n");
        return;
    }

    char buffer[100000];
    ssize_t textsize = read(src, buffer, 100000);

    if (textsize == -1)
    {
        writeToOutput("Error reading from source file\n");
        close(src);
        return;
    }

    while (textsize > 0)
    {
        ssize_t checksize = write(outwrite, buffer, textsize);
        if (checksize != textsize)
        {
            writeToOutput("Error writing to terminal\n"); 
            close(src);
            return;
        }
        textsize = read(src, buffer, 100000);
    }
    // write(STDOUT_FILENO, "\n", 1);

    close(src);
} 

