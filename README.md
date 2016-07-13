# mqLinq
yet another linq implementation in C++

This is another linq implementation in modern C++, based on C++11 and C++14. It is tested on Windows (MSVC 19 on VS2015) and Fedora 24 (clang 3.8 and gcc 6.1.1)
This work is based on @vczh's CppLinq https://github.com/vczh/vczh_toys/tree/master/CppLinq, I rewrote it with new syntax and standard library facilities, and also with some improvements such as SFINAE support.

This repo is just for fulfilling my self-interest and proving my C++ template skill.

## Interfaces
See main.cpp, it contains all test cases.

## Further work
Simplify function declaration with C++14 auto return value feature.

Add more SFINAE support.

Write .natvis file to make a friendlier view in VS-debugger's watch window.
