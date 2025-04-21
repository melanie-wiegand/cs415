void lfcat()
{
/* High level functionality you need to implement: */
/* Get the current directory with getcwd() */
/* Open the dir using opendir() */
/* use a while loop to read the dir with readdir()*/
/* You can debug by printing out the filenames here */
/* Option: use an if statement to skip any names that are not readable
files (e.g. ".", "..", "main.c", "lab2.exe", "output.txt" */
/* Open the file */
/* Read in each line using getline() */
/* Write the line to stdout */
/* write 80 "-" characters to stdout */
/* close the read file and free/null assign your line buffer */
/*close the directory you were reading from using closedir() */
}


#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

void listDir()
{
    /*for the ls command*/

    DIR *curdir;
    // current directory
    curdir = opendir(".");

    if (curdir == NULL) 
    {
        perror("opendir");
        return;
    }

    entry = readdir(curdir);
    while (entry != NULl)
    {
        printf("%s\t", entry->d_name);
        entry = readdir(curdir);
    }

    closedir(curdir);
} 

void showCurrentDir()
{
    /*for the pwd command*/
} 

void makeDir(char *dirName)
{
    /*for the mkdir command*/
} 

void changeDir(char *dirName)
{
    /*for the cd command*/
} 

void copyFile(char *sourcePath, char *destinationPath)
{
    /*for the cp command*/
} 

void moveFile(char *sourcePath, char *destinationPath)
{
    /*for the mv command*/
} 

void deleteFile(char *filename)
{
    /*for the rm command*/
} 

void displayFile(char *filename)
{
    /*for the cat command*/
} 

