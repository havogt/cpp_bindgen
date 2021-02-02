/*
 * GridTools
 *
 * Copyright (c) 2014-2019, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cpp_bindgen/generator.hpp>
#include <cpp_bindgen/handle_impl.hpp>

#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace cpp_bindgen {
    namespace {

        BINDGEN_ADD_GENERATED_DECLARATION(void(), foo);
        BINDGEN_ADD_GENERATED_DECLARATION(bindgen_handle *(int, double const *, bindgen_handle *), bar);
        BINDGEN_ADD_GENERATED_DECLARATION(void(int *const *volatile *const *), baz);
        BINDGEN_ADD_GENERATED_DECLARATION_WRAPPED(void(int, int (&)[1][2][3], std::string const&), qux);

        BINDGEN_ADD_GENERIC_DECLARATION(foo, bar);
        BINDGEN_ADD_GENERIC_DECLARATION(foo, baz);

        const char expected_c_interface[] = R"?(// This file is generated!
#pragma once

#include <cpp_bindgen/array_descriptor.h>
#include <cpp_bindgen/handle.h>
#include <cpp_bindgen/string_descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

bindgen_handle* bar(int, double*, bindgen_handle*);
void baz(int****);
void foo();
void qux(int, bindgen_fortran_array_descriptor*, bindgen_fortran_string_descriptor*);

#ifdef __cplusplus
}
#endif
)?";

        TEST(generator, c_interface) {
            std::ostringstream strm;
            generate_c_interface(strm);
            EXPECT_EQ(strm.str(), expected_c_interface);
        }

        const char expected_fortran_interface[] = R"?(! This file is generated!
module my_module
use iso_c_binding
implicit none
  interface

    type(c_ptr) function bar(arg0, arg1, arg2) bind(c)
      use iso_c_binding
      integer(c_int), value :: arg0
      real(c_double), dimension(*) :: arg1
      type(c_ptr), value :: arg2
    end function
    subroutine baz(arg0) bind(c)
      use iso_c_binding
      type(c_ptr) :: arg0
    end subroutine
    subroutine foo() bind(c)
      use iso_c_binding
    end subroutine
    subroutine qux_impl(arg0, arg1, arg2) bind(c, name="qux")
      use iso_c_binding
      use bindgen_array_descriptor
      use bindgen_string_descriptor
      integer(c_int), value :: arg0
      type(bindgen_fortran_array_descriptor) :: arg1
      type(bindgen_fortran_string_descriptor) :: arg2
    end subroutine

  end interface
  interface foo
    procedure bar, baz
  end interface
contains
    subroutine qux(arg0, arg1, arg2)
      use iso_c_binding
      use bindgen_array_descriptor
      use bindgen_string_descriptor
      integer(c_int), value, target :: arg0
      integer(c_int), dimension(:,:,:), target :: arg1
      character(*), target :: arg2
      type(bindgen_fortran_array_descriptor) :: descriptor1
      type(bindgen_fortran_string_descriptor) :: descriptor2

      descriptor1%rank = 3
      descriptor1%type = 1
      descriptor1%dims = reshape(shape(arg1), &
        shape(descriptor1%dims), (/0/))
      descriptor1%data = c_loc(arg1(lbound(arg1, 1),lbound(arg1, 2),lbound(arg1, 3)))

      descriptor2%data = c_loc(arg2)
      descriptor2%size = len(arg2)

      call qux_impl(arg0, descriptor1, descriptor2)
    end subroutine
end module
)?";

        TEST(generator, fortran_interface) {
            std::ostringstream strm;
            generate_fortran_interface(strm, "my_module");
            EXPECT_EQ(strm.str(), expected_fortran_interface);
        }
        TEST(generator, wrap_short_line) {
            const std::string prefix = "    ";
            const std::string line = "short line, short line";
            EXPECT_EQ(prefix + line + '\n', wrap_line(line, prefix));
        }
        TEST(generator, wrap_almost_full_line) {
            const std::string prefix = "    ";
            const std::string line = std::string(64, 'x') + "," + std::string(63, 'x');
            EXPECT_EQ(prefix + line + '\n', wrap_line(line, prefix));
        }
        TEST(generator, wrap_full_line) {
            const std::string prefix = "    ";
            const std::string line = std::string(64, 'x') + "," + std::string(64, 'x');
            EXPECT_EQ(prefix + std::string(64, 'x') + ", &" + '\n' + prefix + "   " + std::string(64, 'x') + "\n",
                wrap_line(line, prefix));
        }
        TEST(generator, wrap_multiple_lines) {
            const std::string prefix = "    ";
            const std::string line = std::string(50, 'x') + "," + std::string(50, 'x') + "," + std::string(60, 'x') +
                                     "," + std::string(61, 'x') + "," + std::string(60, 'x') + "," +
                                     std::string(62, 'x') + "," + std::string(59, 'x') + "," + std::string(122, 'x');

            const std::string line1 = prefix + std::string(50, 'x') + "," + std::string(50, 'x') + ", &" + '\n';
            const std::string line2 = prefix + "   " + std::string(60, 'x') + "," + std::string(61, 'x') + ", &" + '\n';
            const std::string line3 = prefix + "   " + std::string(60, 'x') + ", &" + '\n';
            const std::string line4 = prefix + "   " + std::string(62, 'x') + "," + std::string(59, 'x') + ", &" + '\n';
            const std::string line5 = prefix + "   " + std::string(122, 'x') + '\n';

            EXPECT_EQ(line1 + line2 + line3 + line4 + line5, wrap_line(line, prefix));
        }
    } // namespace
} // namespace cpp_bindgen
