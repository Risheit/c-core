/**
 * Initializes a dynamic string into [str] from a given null-terminated char
 * array constant [buf], allocating it to the arena [arena]. [buf] is copied
 * until it reaches the null terminator. Calling this function on a non
 * null-terminated string is undefined behaviour.
 *
 * On an error, the [err] field of string is set to a non-zero value.
 */
char* str_createsawf(const char *buf);

