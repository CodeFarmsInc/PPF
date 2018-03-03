PERSISTENT POINTER FACTORY (PPF) - Version 3.4
--------------------------------

PPF helps you to build custom databases, which are fast and can handle
large volume of data efficiently even on an ordinary PC.
PPF is small (30kB object file), and the resulting databases are often by an
order of magnitude faster than ready-made commercial databases. 

PPF gives you three smart pointer classes:
  PersistString  - equivalent of char* for variable length text strings;
  PersistPtr<T>  - equivalent of T* for lightweight class T which does not
                   use inheritance;
  PersistVptr<T> - equivalent of T* for class T which uses inheritance
                   and virtual functions.
If you replace all pointers in your code by these smart pointers,
all objects in your program become automatically persistent (stored on disk).

Note the conceptual difference from persistent objects based on serialization.
Such objects reside in memory, you store them all together, usually at the
end of your program, and they all must me loaded back to memory when you want
to use them again. This is slow, and does not permit you to handle large
data sets efficiently.

Instead of having data in memory all the time, PPF keeps the data on disk,
and pages it to memory on demand. You can retrieve individual objects or small
subsets, have fast access to the data, and since you have full control over
the paging, you can tune up the performance of your database. Objects of
some classes may always be in memory, while other class may never have more
than one object in memory. The size of your disk space is the only limit
on how much data you can store. PPF also provides an transparent management
and reuse of the data space, including free lists. 

                         ------------------
PLATFORM AND COMPILER

If you received a binary version of PPF, it is already customized to the
platform and compiler you ordered. 

If you received the full source of PPF (multi-platform), the source is
set for DOS or Windows (Win95, Win98, WinNT, Windows7) and the Microsoft VC++ compiler, tested up to VC++2007.
If you run under a different environment, edit file factory.h, search for
the text string "DOS", and comment or un-comment #define statements depending
on your environment.
                         ------------------
INSTALLATION

There is no installation, you only compile file factory.cpp.

                         ------------------
LIMITATIONS

Persistent objects can be passed to a function only as a copy, not as
a reference. For more details, including a workaround for situations when
you want to modify the object inside a function call, see the documentation
in docum.htm.

The total size of your data must be under 2^31, which is 2,147,483,648.
If you have larger data, you must compile under 64-bits, and uncomment
the following line in the beginning of the file factory.h:
#define LARGE_DATA_SET

                         ------------------
THIS DIRECTORY

PPF includes the following files:
   factory.h, pointer.cpp - you must include these in your programs.
   factory.obj - you link this file to your program.
   factory.cpp (optional - only if you receive the full source of PPF).

Regression test (regr.bat or regr) is also included. This test runs three
programs - they all should produce message "no errors":
   test1 - testing large data set with lightweight classes;
   test2 - mixture of full C++ classes with lightweight classes;
   test3 - lightweight classes with various forms of string operations.

The documentation is in file docum.htm.

                         ------------------
UNICODE STRINGS

Constructor  PersistString(wchar_t s[]) provides an easy way of creating
a persistent string encoded in UNICODE. The string must be NULL terminated.
This is only an experimental, first implementation of this feature.
When using any function of PersistString which returns (char*), you have
to cast the result as (wchar_t). Functions cmp(),size() and prt() do not
work correctly for UNICODE strings yet -- these functions have not been 
upgraded.

                         ------------------

COPYRIGHT & LEGAL ISSUES

This software is Copyright (C) of Code Farms Inc., 
and is available as open source even for commercial projects or for projects 
that are not open source, assuming the following conditions:

(1) You can keep any number of copies of this software or pass them to your
 friends. However, you must not place it on internet for download or automatic
 delivery.

(2) If you use this software in your product, any description, documentation,
 press release, publication, or reference to your product which is longer than
 100 words must acknowledge this fact. For example: 
    "This product is based on Code Farms DOL library" or 
    "This software was developed with Code Farms PPF library".

(3) If you improve the software, expand it or develop with it a product which
 serves the same purpose, the new product must NOT be distributed under
 a new name, but as a "version" of the original software. For example, if you
 develop a new class library based on InCode library, your new library must
 be called "XYZ version of InCode library", where XYZ is your name or the name
 of your new product or your company.

(4) Code Farms provides no warranty. The user takes the responsibility for any
 direct or indirect damages caused by this software. 

