#include <QtCore/qglobal.h>

#define QCOMPILERDETECTION_H

#define Q_NULLPTR         nullptr
#define Q_DECL_EQ_DEFAULT
#define Q_DECL_EQ_DELETE = delete
#define Q_DECL_CONSTEXPR constexpr
#define Q_DECL_RELAXED_CONSTEXPR constexpr
#define Q_CONSTEXPR constexpr
#define Q_RELAXED_CONSTEXPR constexpr
	
#define Q_DECL_OVERRIDE override
#define Q_DECL_FINAL final



#define Q_CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#define Q_CC_MSVC (_MSC_VER)
#define Q_CC_MSVC_NET
#define Q_OUTOFLINE_TEMPLATE inline
#define Q_COMPILER_MANGLES_RETURN_TYPE
#define Q_FUNC_INFO __FUNCSIG__
#define Q_ALIGNOF(type) __alignof(type)
#define Q_DECL_ALIGN(n) __declspec(align(n))
#define Q_ASSUME_IMPL(expr) __assume(expr)
#define Q_UNREACHABLE_IMPL() __assume(0)
#define Q_NORETURN __declspec(noreturn)
#define Q_DECL_DEPRECATED __declspec(deprecated)
#define Q_DECL_DEPRECATED_X(text) __declspec(deprecated(text))
#define Q_DECL_EXPORT __declspec(dllexport)
#define Q_DECL_IMPORT __declspec(dllimport)
#define QT_MAKE_UNCHECKED_ARRAY_ITERATOR(x) stdext::make_unchecked_array_iterator(x)
#define QT_MAKE_CHECKED_ARRAY_ITERATOR(x, N) stdext::make_checked_array_iterator(x, size_t(N))
#define Q_DECL_VARIABLE_DEPRECATED
#define Q_CC_INTEL  __INTEL_COMPILER
#define Q_DECL_NOTHROW  throw()


#define Q_COMPILER_DEFAULT_DELETE_MEMBERS







#define Q_DECL_NOEXCEPT noexcept
#define Q_DECL_NOEXCEPT_EXPR(x) noexcept(x)

#define Q_ALIGNOF(x)  alignof(x)

#define Q_DECL_ALIGN(n)   alignas(n)


#define Q_NORETURN
#define Q_LIKELY(x) (x)
#define Q_UNLIKELY(x) (x)
#define Q_ASSUME_IMPL(expr) qt_noop()
#define Q_UNREACHABLE_IMPL() qt_noop()


#define qMove(x) std::move(x)


#define Q_UNREACHABLE() \
    do {\
        Q_ASSERT_X(false, "Q_UNREACHABLE()", "Q_UNREACHABLE was reached");\
        Q_UNREACHABLE_IMPL();\
    } while (false)

#define Q_ASSUME(Expr) \
    do {\
        const bool valueOfExpression = Expr;\
        Q_ASSERT_X(valueOfExpression, "Q_ASSUME()", "Assumption in Q_ASSUME(\"" #Expr "\") was not correct");\
        Q_ASSUME_IMPL(valueOfExpression);\
    } while (false)


#endif // QCOMPILERDETECTION_H
