#ifndef UTILS_H
#define UTILS_H

class Utils {
public:
    // Case-insensitive substring search (same behavior as your original strcasestr_custom)
    static char* strcasestr_custom(const char* haystack, const char* needle);
};

#endif // UTILS_H
