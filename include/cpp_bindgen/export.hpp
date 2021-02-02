/*
 * GridTools
 *
 * Copyright (c) 2014-2019, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <boost/preprocessor.hpp>

#include "common/function_traits.hpp"
#include "function_wrapper.hpp"
#include "generator.hpp"

#define BINDGEN_EXPORT_BINDING_IMPL_PARAM_DECL(z, i, signature) \
    typename std::tuple_element<i,                              \
        ::cpp_bindgen::function_traits::parameter_types<::cpp_bindgen::wrapped_t<signature>>::type>::type param_##i

#define BINDGEN_ADD_GENERATED_DEFINITION_IMPL(n, name, cppsignature, impl)                                        \
    static_assert(::cpp_bindgen::function_traits::arity<cppsignature>::value == n, "arity mismatch");             \
    extern "C" typename ::cpp_bindgen::function_traits::result_type<::cpp_bindgen::wrapped_t<cppsignature>>::type \
    name(BOOST_PP_ENUM(n, BINDGEN_EXPORT_BINDING_IMPL_PARAM_DECL, cppsignature)) {                                \
        return ::cpp_bindgen::wrap<cppsignature>(impl)(BOOST_PP_ENUM_PARAMS(n, param_));                          \
    }

/**
 *   Defines the function with the given name with the C linkage.
 *
 *   The signature if the generated function will be transformed from the provided signature according to the following
 *   rules:
 *     - for result type:
 *       - `void` and arithmetic types remain the same;
 *       - classes (and structures) and references to them are transformed to the pointer to the opaque handle
 *         (`bindgen_handle*`) which should be released by calling `void bindgen_release(bindgen_handle*)` function;
 *       - all other result types will cause a compiler error.
 *     - for parameter types:
 *       - arithmetic types and pointers to them remain the same;
 *       - references to arithmetic types are transformed to the corresponded pointers;
 *       - types that fulfill the concept of being fortran_array_bindable are transformed to a
 *         bindgen_fortran_array_descriptor
 *       - classes (and structures) and references or pointers to them are transformed to `bindgen_handle*`;
 *       - all other parameter types will cause a compiler error.
 *   Additionally the newly generated function will be registered for automatic interface generation.
 *
 *   @param n The arity of the generated function.
 *   @param name The name of the generated function.
 *   @param cppsignature The signature that will be used to invoke `impl`.
 *   @param impl The functor that the generated function will delegate to.
 */
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(n, name, cppsignature, impl) \
    BINDGEN_ADD_GENERATED_DEFINITION_IMPL(n, name, cppsignature, impl)     \
    BINDGEN_ADD_GENERATED_DECLARATION(::cpp_bindgen::wrapped_t<cppsignature>, name)

/**
 *   Defines the function with the given name with the C linkage with an additional wrapper in the fortran bindings. The
 *   additional wrapper provides the following functionality:
 *     - It generates a bindgen_fortran_array_descriptor from an array, if the type on C++-side is
 *       fortran_array_wrappable.
 *
 *   The signature if the generated function will be transformed from the provided signature according to the following
 *   rules:
 *     - for result type:
 *       - `void` and arithmetic types remain the same;
 *       - classes (and structures) and references to them are transformed to the pointer to the opaque handle
 *         (`bindgen_handle*`) which should be released by calling `void bindgen_release(bindgen_handle*)` function;
 *       - all other result types will cause a compiler error.
 *     - for parameter types:
 *       - arithmetic types and pointers to them remain the same;
 *       - references to arithmetic types are transformed to the corresponded pointers;
 *       - types that are fortran_array_bindable but not fortran_array_wrappable are transformed to a
 *         bindgen_fortran_array_descriptor
 *       - types that are fortran_array_wrappable are transformed to a bindgen_fortran_array_descriptor in the
 *         c-bindings, and provide a wrapper in the fortran-bindings such that they can be called with a fortran array
 *       - types that are fortran_string_bindable are transformed to a bindgen_fortran_string_descriptor in the
 *         c-bindings, and provide a wrapper in the fortran-bindings such that they can be called with a fortran string
 *       - classes (and structures) and references or pointers to them are transformed to `bindgen_handle*`;
 *       - all other parameter types will cause a compiler error.
 *   Additionally the newly generated function will be registered for automatic interface generation.
 *
 *   A note about fortran strings support:
 *     `std::string` (and `std::string_view` for C++ standard >= 17) and their R-refs and const L-refs satisfy
 *     `fortran_string_bindable` criteria. This means that for the C++ function like `void f(std::string& const)`
 *     fortran wrapper will be generated that takes `character(*)` as a parameter. Note that in this case additional
 *     data copy will take place while calling `std::string` constructor. For C++17 and higher the user can use
 *     `void f(std::string_view)` signature to avoid data copy. For the C++11/C++14 there are also copy free solutions:
 *       - use f(bindgen_fortran_string_descriptor) signature for the raw access to the fortran data or
 *       - use custom `string_view like` class (T) and provide
 *          `T bindgen_make_fortran_string_view(bindgen_fortran_string_descriptor desc, T*);` function available by ADL.
 *
 *   @param n The arity of the generated function.
 *   @param name The name of the generated function.
 *   @param cppsignature The signature that will be used to invoke `impl`.
 *   @param impl The functor that the generated function will delegate to.
 */
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(n, name, cppsignature, impl) \
    BINDGEN_ADD_GENERATED_DEFINITION_IMPL(n, name, cppsignature, impl)             \
    BINDGEN_ADD_GENERATED_DECLARATION_WRAPPED(cppsignature, name)

/// The flavour of BINDGEN_EXPORT_BINDING_WITH_SIGNATURE where the `impl` parameter is a function pointer.
#define BINDGEN_EXPORT_BINDING(n, name, impl) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(n, name, decltype(BOOST_PP_REMOVE_PARENS(impl)), impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED(n, name, impl) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(n, name, decltype(BOOST_PP_REMOVE_PARENS(impl)), impl)

#define BINDGEN_EXPORT_GENERIC_BINDING_IMPL_IMPL(generatorsuffix, n, generic_name, concrete_name, impl) \
    BOOST_PP_CAT(BINDGEN_EXPORT_BINDING, generatorsuffix)(n, concrete_name, impl);                      \
    BINDGEN_ADD_GENERIC_DECLARATION(generic_name, concrete_name)

#define BINDGEN_EXPORT_GENERIC_BINDING_IMPL(generatorsuffix, n, name, suffix, impl) \
    BINDGEN_EXPORT_GENERIC_BINDING_IMPL_IMPL(generatorsuffix, n, name, BOOST_PP_CAT(name, suffix), impl)

#define BINDGEN_EXPORT_GENERIC_BINDING_IMPL_FUNCTOR(r, data, i, elem)    \
    BINDGEN_EXPORT_GENERIC_BINDING_IMPL(BOOST_PP_TUPLE_ELEM(4, 0, data), \
        BOOST_PP_TUPLE_ELEM(4, 1, data),                                 \
        BOOST_PP_TUPLE_ELEM(4, 2, data),                                 \
        i,                                                               \
        (BOOST_PP_TUPLE_ELEM(4, 3, data) < BOOST_PP_REMOVE_PARENS(elem) >));

#define BINDGEN_EXPORT_GENERIC_BINDING(n, name, impl_template, template_params) \
    BOOST_PP_SEQ_FOR_EACH_I(BINDGEN_EXPORT_GENERIC_BINDING_IMPL_FUNCTOR,        \
        (, n, name, impl_template),                                             \
        BOOST_PP_VARIADIC_SEQ_TO_SEQ(template_params))                          \
    static_assert(1, "")

#define BINDGEN_EXPORT_GENERIC_BINDING_WRAPPED(n, name, impl_template, template_params) \
    BOOST_PP_SEQ_FOR_EACH_I(BINDGEN_EXPORT_GENERIC_BINDING_IMPL_FUNCTOR,                \
        (_WRAPPED, n, name, impl_template),                                             \
        BOOST_PP_VARIADIC_SEQ_TO_SEQ(template_params))                                  \
    static_assert(1, "")

/// BINDGEN_EXPORT_BINDING_WITH_SIGNATURE shortcuts for the given arity
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_0(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(0, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_1(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(1, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_2(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(2, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_3(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(3, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_4(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(4, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_5(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(5, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_6(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(6, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_7(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(7, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_8(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(8, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_9(name, s, i) BINDGEN_EXPORT_BINDING_WITH_SIGNATURE(9, name, s, i)

#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_0(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(0, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_1(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(1, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_2(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(2, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_3(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(3, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_4(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(4, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_5(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(5, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_6(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(6, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_7(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(7, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_8(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(8, name, s, i)
#define BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED_9(name, s, i) \
    BINDGEN_EXPORT_BINDING_WITH_SIGNATURE_WRAPPED(9, name, s, i)

/// BINDGEN_EXPORT_BINDING shortcuts for the given arity
#define BINDGEN_EXPORT_BINDING_0(name, impl) BINDGEN_EXPORT_BINDING(0, name, impl)
#define BINDGEN_EXPORT_BINDING_1(name, impl) BINDGEN_EXPORT_BINDING(1, name, impl)
#define BINDGEN_EXPORT_BINDING_2(name, impl) BINDGEN_EXPORT_BINDING(2, name, impl)
#define BINDGEN_EXPORT_BINDING_3(name, impl) BINDGEN_EXPORT_BINDING(3, name, impl)
#define BINDGEN_EXPORT_BINDING_4(name, impl) BINDGEN_EXPORT_BINDING(4, name, impl)
#define BINDGEN_EXPORT_BINDING_5(name, impl) BINDGEN_EXPORT_BINDING(5, name, impl)
#define BINDGEN_EXPORT_BINDING_6(name, impl) BINDGEN_EXPORT_BINDING(6, name, impl)
#define BINDGEN_EXPORT_BINDING_7(name, impl) BINDGEN_EXPORT_BINDING(7, name, impl)
#define BINDGEN_EXPORT_BINDING_8(name, impl) BINDGEN_EXPORT_BINDING(8, name, impl)
#define BINDGEN_EXPORT_BINDING_9(name, impl) BINDGEN_EXPORT_BINDING(9, name, impl)

#define BINDGEN_EXPORT_BINDING_WRAPPED_0(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(0, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_1(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(1, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_2(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(2, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_3(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(3, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_4(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(4, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_5(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(5, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_6(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(6, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_7(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(7, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_8(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(8, name, impl)
#define BINDGEN_EXPORT_BINDING_WRAPPED_9(name, impl) BINDGEN_EXPORT_BINDING_WRAPPED(9, name, impl)
