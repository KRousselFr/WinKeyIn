windres -i WinKeyIn.rc -o WinKeyInRes.o
gcc -std=c11 -c WinKeyIn.c -o WinKeyIn.o -Wall -Wextra -Wconversion -pedantic
gcc WinKeyIn.o WinKeyInRes.o -o WinKeyIn.exe -Wall -Wextra -O -mwindows
strip WinKeyIn.exe
