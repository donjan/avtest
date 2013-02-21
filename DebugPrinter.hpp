/*******************************************************************************
 * 
 * DebugPrinter.hpp
 * Unostentatious printing in a nice format, and some more
 * Written by Donjan Rodic, 2011-2012, for free use
 * 
 * Creates static object dcout and macro _HERE_
 * Link with -rdynamic in order to properly get stack() function names
 * Pass NOEXECINFO flag to compiler on Windows (disables stack() method)
 * Usage:
 *   dcout << "foo";
 *   dcout << "foo ", 5, " bar";
 *   dcout.stack();
 *   dcout(__LINE__);
 *   dcout(anything);             // anything must work with ostream::operator<<
 *   dcout(__func__, __LINE__);
 *   dcout(label, anything);      // label must work with ostream::operator<<
 *   dcout(_HERE_);               // useful shortcut: dcout(__func__, __LINE__)
 * 
 * Precision of float output (default = 5) can be then set with
 *   dcout.precision = 12;
 * 
 ******************************************************************************/

#pragma once

#include <iostream>
#include <iomanip>

#ifndef NOEXECINFO
#include <cstdlib>
#include <execinfo.h>
#endif // NOEXECINFO


class DebugPrinter {

  public:

  int precision;

  DebugPrinter() : precision(5) {}


  template <typename T>
  inline DebugPrinter & operator<<(const T& output) {
    size_t savep = (size_t)std::cout.precision();
    std::ios_base::fmtflags savef =
                   std::cout.setf(std::ios_base::fixed, std::ios::floatfield);
    std::cout << std::setprecision(precision) << std::fixed << output
              << std::setprecision(savep);
    std::cout.setf(savef, std::ios::floatfield);
    std::cout.flush();
    return *this;
  }

  template <typename T>
  inline DebugPrinter & operator,(const T& output) { return operator<<(output); }

  template <typename T, typename U>
  inline void operator()(const T& label, const U& line) const {
    std::cout << "\033[0;36m" << ">>> " << label << ": " << line << "\033[0m" << std::endl;
    std::cout.flush();
  }
  template <typename T>
  inline void operator()(const T& line) const { operator()("checkpoint", line); }

  #ifndef NOEXECINFO
  void stack() const {
    void * stack[max_backtrace];
    std::size_t r = backtrace(stack, max_backtrace);
    char ** symbols = backtrace_symbols(stack, r);
    if(!symbols) return;
    std::cout << "Obtained " << r << " stack frames:" << std::endl;
    for (std::size_t i = 0; i < r; ++i) {
      std::cout << " " << symbols[i] << std::endl;
    }
    std::cout.flush();
    free(symbols);
  }
  #endif // NOEXECINFO


  private:
  static const unsigned int max_backtrace = 50;

};

static DebugPrinter dcout;

#define _HERE_ __func__, __LINE__
