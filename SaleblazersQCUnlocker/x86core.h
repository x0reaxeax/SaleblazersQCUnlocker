#ifndef _HAMMERHILL_X86CORE_H_
#define _HAMMERHILL_X86CORE_H_

#define GLOBAL
#ifndef EXTERN
#define EXTERN              extern
#endif // EXTERN

#define VOLATILE            volatile
#define STATIC              static
#define INLINE              inline
#ifdef __GNUC__
#define NAKED               __attribute__((naked))
#define UNUSED              __attribute__((unused))
#endif // __GNUC__
#ifdef _MSC_VER
#define NAKED               __declspec(naked)
#define UNUSED
#endif // _MSC_VER
#define MAYBE_UNUSED        UNUSED
#define UNREFERENCED_PARAM  UNUSED
#ifdef __GNUC__
#define PURE                __attribute_pure__
#define CPACKED             __attribute__((packed))
#define FORCE_INLINE        __attribute__((always_inline))
#define ALIGN(x)            __attribute__((aligned(x)))
#define NORETURN            __attribute__((noreturn))
#endif // __GNUC__
#ifdef _MSC_VER
#define PURE
#define CPACKED             __pragma(pack(push, 1)) __pragma(pack(pop))
#define FORCE_INLINE        __forceinline
#define ALIGN(x)            __declspec(align(x))
#define NORETURN            __declspec(noreturn)
#endif // _MSC_VER
#ifndef CONST
#define CONST               const
#endif // CONST

#ifdef __INTEL_LLVM_COMPILER
#define __ASM               __asm
#define __VOLATILE          
#endif // __INTEL_LLVM_COMPILER

#endif // _HAMMERHILL_X86CORE_H_