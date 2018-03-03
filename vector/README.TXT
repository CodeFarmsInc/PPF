Alfred,
This is the first version of the vector class. It compiles,
it is functional, but only the main features have been tested. 
I to you to test all the insert, append, and remove functions.
I am fairly confident they will work, and if not I will fix them 
promptly -- assuming you'll give me a simple(!) example when 
things don't work.

All these files should be in subdirectory ppf\vector,
except for the file factory.cpp, which is a small improvement
of factory.cpp from the latest version of PPF.

The class vector is all in file vector.h, there is no file vector.cpp.
It does not need any documentation, it should be clear from the code
what the interface does. 

The attached code testvect.cpp with files testA.* and testB.*
demonstrate the use of the vector class.
Assuming that you are running from the CMD (DOS) window,
tt.bat compiles testvect.exe
r1.bat makes the first vector with 6 entries, and stores it in the DB.
r2.bat adds 6 more entries, increasing the size of the vector, and stores it.
r3.bat does the same thing but does not store, DB remains as it was.

Comments:
-- The test shows files *.ppf (essentially the database) after each run.
   You can watch how the files are changing and that they are indeed
   very small in size. 
-- Adding 6 more entries could also be accomplished by calling 6-times
   the appropriate append function.
-- Even though I am quite familiar with iterators and their design,
   I did not implement the STL interface which depends heavily on them.
   It makes no sense: If you use a vector, you think in integer indexes
   and not in some abstract iterators which only confuse algorithms.
   I believe that the old argument about compatibility of interface
   does not apply here; this is the case of apples and oranges,
   and the software architect should think ahead of time which 
   alternatives applies to her application.

Good luck, and let me know how this works/does not work for you.
Jiri, July 15, 2008
