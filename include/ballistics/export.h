#ifndef BALLISTICS_EXPORT_H
#define BALLISTICS_EXPORT_H

#if defined(_WIN32)
#if defined(BALLISTICS_BUILD_SHARED)
#define BALLISTICS_API __declspec(dllexport)
#elif defined(BALLISTICS_USE_SHARED)
#define BALLISTICS_API __declspec(dllimport)
#else
#define BALLISTICS_API
#endif
#else
#define BALLISTICS_API __attribute__((visibility("default")))
#endif

#endif
