del *.exe
del *.obj
del *.ppf
cl /c /EHsc factory.cpp
cl /EHsc test1.cpp test1a.cpp test1b.cpp factory.obj
test1 1
test1 2
test1 3
del *.ppf
cl /EHsc test2.cpp test2a.cpp test2b.cpp factory.obj
test2 1
test2 2
del *.ppf
cl /EHsc test3.cpp factory.obj
test3 1
dir pstring.ppf > strsize1
test3 2
test3 3
test3 4
dir pstring.ppf > strsize2
diff =7 strsize1 strsize2
del *.ppf
