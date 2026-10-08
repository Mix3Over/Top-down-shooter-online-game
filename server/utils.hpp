#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

template <typename... Args> /* This mimics GNU 'error' function */
void error(int status, int errnum, const char *format, Args... args) {
    fprintf(stderr, (format + std::string(errnum ? ": %s\n" : "\n")).c_str(), args..., strerror(errnum));
    if (status) exit(status);
}

int getRandomNumberInRange(int min, int max);