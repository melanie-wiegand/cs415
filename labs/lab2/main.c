#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    int filemode = 0;

    // if -f flag is used, start file mode
    if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        // update file mode bool
        filemode = 1;  
        printf("File mode:\n");

        // open provided file
        FILE *file = fopen(argv[2], "r");
        if (file == NULL)
        {
            perror("Could not open file: %s\n", argv[2]);
            return 1;
        }
    }

    // if no flags, start interactive mode
    else if (argc == 1) 
    {
        // keep filemode bool at 0
        printf("Interactive mode:\n");

        // collect input from user
        input = stdin;  
    }

    // otherwise, give error message
    else 
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s           (for interactive mode)\n", argv[0]);
        fprintf(stderr, "  %s -f <file> (for file mode)\n", argv[0]);
        return 1;
    }

    if (filemode)
    {
        fclose(file);
    }

    return 0;
}