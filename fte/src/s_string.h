#ifndef S_STRING_H
#define S_STRING_H

#include <stddef.h>

size_t UnTabStr(char *dest, size_t maxlen, const char *source, size_t slen);
size_t UnEscStr(char *dest, size_t maxlen, const char *source, size_t slen);

#if !defined(HAVE_STRLCPY)
size_t strlcpy(char *dst, const char *src, size_t size);
#endif // !HAVE_STRLCPY

#if !defined(HAVE_STRLCAT)
size_t strlcat(char *dst, const char *src, size_t size);
#endif // !HAVE_STRLCAT

#endif // S_STRING_H
