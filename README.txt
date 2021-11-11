==Description==
This software simulates a file system on a computer with a small disk and a single folder.
This program implements all the operations that an operating system performs on a disk.

functions:
constructor - Opens a file that simulates the disk and initializes it to '\0' as the number of times it was set.
In addition, the constructor initializes the fields.
There are 5 main methods:
Function 1(fsFormat) - This method gets a number and formats the disk so that the size of each block is the size of that number.
Function 2(CreateFile) - This method creates a new file and allocates disk space to a management block.
Function 3(WriteToFile) - This method writes characters into a selected file.
Function 4(ReadFromFile) - This method reads the contents of the selected file.
Function 5(DelFile) - This method deletes a file.

==Program Files==
ex7_final_proj.2021.cpp

==How to compile?==
compile: g++ ex7_final_proj.2021.cpp -o ex7
run: ./ex7

==Input:==
By using the application operations can be performed on the disk.(format, write, read, open, close, delete etc.)

==Output:==
The list of files and the fd of each file.
Disk contents.