cl /EHsc test3.cpp factory.obj
test3 1
dir pstring.ppf > strsize1
test3 2
test3 3
test3 4
dir pstring.ppf > strsize2
diff =7 strsize1 strsize2
