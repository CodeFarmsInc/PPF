cd ..\..
del factory.obj
cl /c factory.cpp 
cd vector\testemb
cl /EHsc testemb.cpp testA.cpp testE.cpp ..\..\factory.obj
