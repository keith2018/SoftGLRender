/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#   undef  PLATFORM_WINDOWS
#   define PLATFORM_WINDOWS
#elif defined(__ANDROID__)
#	undef  PLATFORM_ANDROID
#	define PLATFORM_ANDROID 1
#elif  defined(__linux__)
#	undef  PLATFORM_LINUX
#	define PLATFORM_LINUX 1
#elif  defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#	undef  PLATFORM_IOS
#	define PLATFORM_IOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#	undef  PLATFORM_OSX
#	define PLATFORM_OSX 1
#elif defined(__EMSCRIPTEN__)
#	undef  PLATFORM_EMSCRIPTEN
#	define PLATFORM_EMSCRIPTEN 1
#endif
