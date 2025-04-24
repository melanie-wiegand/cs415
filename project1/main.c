#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h" 
#include "command.h"

int main(int argc, char* argv[])
{
    FILE *input = NULL;
    FILE *output = NULL;
    int filemode = 0;

    // if -f flag is used, start file mode
    if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        // update file mode bool
        // filemode = 1;  
        // printf("File mode:\n");

        // take input from provided file
        input = fopen(argv[2], "r");
        if (input == NULL)
        {
            perror("Could not open input file");
            return 1;
        }

        // write output to file
        output = fopen("output.txt", "w");
        if (!output) {
            perror("Error opening output.txt");
            fclose(input);
            return 1;
        }
    }

    // if no flags, start interactive mode
    else if (argc == 1) 
    {
        // keep filemode bool at 0
        // printf("Interactive mode:\n");

        // collect input from user
        input = stdin;  
        // write to terminal
        output = stdout;
    }

    // otherwise, give error message
    else 
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\t%s\t\t\t(for interactive mode)\n", argv[0]);
        fprintf(stderr, "\t%s -f <file>\t(for file mode)\n", argv[0]);
        return 1;
    }


    // command loop
    char *line = NULL;
    size_t len = 0;

    while (1) {

        // interactive mode prompt
        if (!filemode) {
            fprintf(output, ">>> ");
            fflush(output);
        }

        if (getline(&line, &len, input) == -1) {
            break; 
        }

        char *split;
        // separate inputs by semicolon delimeter
        char *curcmd = strtok_r(line, ";", &split);

        while (curcmd != NULL)
        {
            // tokenize input
            command_line cmd = str_filler(curcmd, " \t\n");

            if (cmd.num_token == 0) 
            {
                free_command_line(&cmd);
                curcmd = strtok_r(NULL, ";", &split);
                continue;
            }

        
            // quit out of pseudoshell
            else if (strcmp(cmd.command_list[0], "exit") == 0) {
                return 1;
            }


            // list items in current dir
            else if (strcmp(cmd.command_list[0], "ls") == 0) {
                if (cmd.num_token == 1)
                {
                    listDir();
                }
                else
                {
                    fprintf(stderr, "Error! Unsupported parameters for command: ls\n");
                }
                
            }


            // show current path
            else if (strcmp(cmd.command_list[0], "pwd") == 0) {
                if (cmd.num_token == 1)
                {
                    showCurrentDir();
                }
                else
                {
                    fprintf(stderr, "Error! Unsupported parameters for command: pwd\n");
                }
                
            }

            // create directory
            else if (strcmp(cmd.command_list[0], "mkdir") == 0) {
                if (cmd.num_token == 2)
                {
                    makeDir(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tmkdir <dirName>\n");
                }
                
            }


            // change to specified directory
            else if (strcmp(cmd.command_list[0], "cd") == 0) {
                if (cmd.num_token == 2)
                {
                    changeDir(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcd <dirName>\n");
                }
            }

            // copy file to new path
            else if (strcmp(cmd.command_list[0], "cp") == 0) {
                if (cmd.num_token == 3)
                {
                    //use basename
                    copyFile(cmd.command_list[1], cmd.command_list[2]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcp <src> <dst>\n");
                }
            }

            
            // move or rename file
            else if (strcmp(cmd.command_list[0], "mv") == 0) {
                if (cmd.num_token == 3)
                {
                    moveFile(cmd.command_list[1], cmd.command_list[2]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tmv <src> <dst>\n");
                }
            }


            // delete file
            else if (strcmp(cmd.command_list[0], "rm") == 0) {
                if (cmd.num_token == 2)
                {
                    deleteFile(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\trm <file>\n");
                }
            }


            // display file contents
            else if (strcmp(cmd.command_list[0], "cat") == 0) {
                if (cmd.num_token == 2)
                {
                    displayFile(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcat <file>\n");
                }
            }

            else
            {
                fprintf(stderr, "Error! Unrecognized command: ");
                printf("%s\n", cmd.command_list[0]);
                break;
            }

            free_command_line(&cmd);
            // go to next cmd
            curcmd = strtok_r(NULL, ";", &split);
        }
    }

    free(line);

    if (filemode)
    {
        fclose(input);
    }

    return 0;
}