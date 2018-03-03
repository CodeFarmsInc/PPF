cd ..
del factory.obj
cl /EHsc /c factory.cpp 
cd vector
cl /EHsc testvect.cpp testA.cpp testB.cpp ..\factory.obj
