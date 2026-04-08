#pragma once

#include "stdio.h"
#include <windows.h>

void PrintHResult(HRESULT hr)
{
    char *message = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   hr,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&message,
                   0,
                   NULL);

    if (message)
    {
        fprintf(stderr, "HRESULT 0x%08X: %s\n", hr, message);
        LocalFree(message);
    }
    else
    {
        fprintf(stderr, "HRESULT 0x%08X: unknown error\n", hr);
    }
}

#ifdef DEBUG_BUILD
#define err(format, ...)                                                       \
    {                                                                          \
        fprintf(stderr,                                                        \
                "%s -> %s -> %i [ERROR]: " format "\n",                        \
                __FILE__,                                                      \
                __FUNCTION__,                                                  \
                __LINE__,                                                      \
                ##__VA_ARGS__);                                                \
        system("pause");                                                       \
        exit(1);                                                               \
    }
#else
#define err(format, ...)                                                       \
    {                                                                          \
        fprintf(stderr,                                                        \
                "%s -> %s -> %i [ERROR]: " format "\n",                        \
                __FILE__,                                                      \
                __FUNCTION__,                                                  \
                __LINE__,                                                      \
                ##__VA_ARGS__);                                                \
        exit(1);                                                               \
    }
#endif

#ifdef DEBUG_BUILD
#define debug(format, ...)                                                     \
    {                                                                          \
        printf("%s [DEBUG]: " format "\n", __FUNCTION__, ##__VA_ARGS__);       \
    }
#else
#define debug(format, ...)
#endif

#ifdef DEBUG_BUILD
#define validate(hr, msg)                                                      \
    {                                                                          \
        HRESULT _hr = (hr);                                                    \
        if (FAILED(_hr))                                                       \
        {                                                                      \
            fprintf(stderr,                                                    \
                    "%s -> %s -> %i [FATAL]: %s\n",                            \
                    __FILE__,                                                  \
                    __FUNCTION__,                                              \
                    __LINE__,                                                  \
                    msg);                                                      \
            PrintHResult(_hr);                                                 \
            system("pause");                                                   \
            exit(1);                                                           \
        }                                                                      \
    }
#else
#define validate(hr, msg)                                                      \
    {                                                                          \
        HRESULT _hr = (hr);                                                    \
        if (FAILED(_hr))                                                       \
        {                                                                      \
            fprintf(stderr,                                                    \
                    "%s -> %s -> %i [FATAL]: %s\n",                            \
                    __FILE__,                                                  \
                    __FUNCTION__,                                              \
                    __LINE__,                                                  \
                    msg);                                                      \
            PrintHResult(_hr);                                                 \
            exit(1);                                                           \
        }                                                                      \
    }
#endif