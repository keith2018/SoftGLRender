/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#ifdef _MSC_VER
#define MM_F32(v, i) v.m128_f32[i]
#else
#define MM_F32(v, i) v[i]
#endif

#define PTR_ADDR(p) ((size_t)(p))
