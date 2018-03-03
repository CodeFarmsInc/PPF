cd ..
del factory.obj
cl /c /EHsc factory.cpp 
cd lib
cl /EHsc test1col.cpp ..\factory.obj
