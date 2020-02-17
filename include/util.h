#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

/* Create dynamically allocated copy of null-terminated string and return it.
 */
char *str_create_copy(const char *string);

/* Create dynamically allocated string which is concatenation of string1 and
 * string2.
 */
char *str_create_concat2(const char *string1, const char *string2);

/* Create dynamically allocated string which is concatenation of string1,
 * string2 and string3.
 */
char *str_create_concat3(const char *string1, const char *string2,
			 const char *string3);

#endif

