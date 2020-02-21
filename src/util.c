#include "util.h"

#include <stdlib.h>
#include <string.h>

char*
str_create_copy(const char* string)
{
    const size_t length = strlen(string) + 1;
    char* const copy = malloc(length);
    memcpy(copy, string, length);
    return copy;
}

char*
str_create_concat2(const char* string1, const char* string2)
{
    const size_t length1 = strlen(string1);
    const size_t length2 = strlen(string2);
    const size_t length = length1 + length2 + 1;
    char* const result = malloc(length);
    strcpy(result, string1);
    strcpy(result + length1, string2);
    return result;
}

char*
str_create_concat3(const char* string1,
                   const char* string2,
                   const char* string3)
{
    const size_t length1 = strlen(string1);
    const size_t length2 = strlen(string2);
    const size_t length3 = strlen(string3);
    const size_t length = length1 + length2 + length3 + 1;
    char* const result = malloc(length);
    strcpy(result, string1);
    strcpy(result + length1, string2);
    strcpy(result + length1 + length2, string3);
    return result;
}

