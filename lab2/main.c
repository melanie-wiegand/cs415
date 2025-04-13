#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    // if no flags, start interactive mode
    if (argc == 1) 
    {
        printf("Interactive mode:\n");
    }
    else if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        char* FILE = argv[2];
        printf("File mode:\n");
    }
    else 
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s           (for interactive mode)\n", argv[0]);
        fprintf(stderr, "  %s -f <file> (for file mode)\n", argv[0]);
        return 1;
    }
    return 0;
}