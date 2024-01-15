#include <stdio.h>

struct ali {
    char c;
    int i;
};

int main()
{
    printf("The alignment of 'int' is: %zu\n", alignof(int));
    printf("The alignment of 'double' is: %zu\n", alignof(double));
    printf("The alignment of a 'struct' is: %zu\n", alignof(ali));

    return 0;
}