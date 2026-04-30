#include "utils.h"
#include <string.h>
#include <strings.h> // for strncasecmp

// Implementation of the case-insensitive strstr used by the handshake parser
char* Utils::strcasestr_custom(const char* haystack, const char* needle) {
    size_t needle_len = strlen(needle);
    for (; *haystack; ++haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char*)haystack;
        }
    }
    return NULL;
}

