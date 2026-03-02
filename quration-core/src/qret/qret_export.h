/**
 * @file qret/qret_export.h
 * @brief Define QRET_EXPORT macro.
 */

#ifndef QRET_QRET_EXPORT_H
#define QRET_QRET_EXPORT_H

#ifdef QRET_BUILD_PYTHON
#define QRET_EXPORT
#else
// not QRET_BUILD_PYTHON

#if defined(_WIN32)
#if defined(QRET_BUILDING_DLL)
#define QRET_EXPORT __declspec(dllexport)
#else
#define QRET_EXPORT __declspec(dllimport)
#endif
#else
// not _WIN32
#define QRET_EXPORT __attribute__((visibility("default")))
#endif  // _WIN32

#endif  // QRET_BUILD_PYTHON

#endif  // QRET_QRET_EXPORT_H
