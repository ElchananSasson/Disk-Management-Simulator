#include <iostream>
#include <vector>
#include <assert.h>
#include <fcntl.h>
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
#define DISK_SIZE 256

using namespace std;

class FsFile {
    int file_size;
    int block_in_use;
    int numOfChars;
    int index_block;
    int block_size;

public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        numOfChars = 0;
        block_size = _block_size;
        index_block = -1;
    }

    int getBlockInUse() {
        return block_in_use;
    }

    void setBlockInUse(int n) {
        block_in_use = n;
    }

    int getNumOfChars() {
        return numOfChars;
    }

    void setNumOfChars(int n) {
        numOfChars = n;
    }

    int getIndexBlock() {
        return index_block;
    }

    void setIndexBlock(int n) {
        index_block = n;
    }
};

class FileDescriptor {
    string file_name;
    FsFile * fs_file;
    bool inUse;

public:
    FileDescriptor(string FileName, FsFile * fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    string getFileName() {
        return file_name;
    }

    FsFile * getFsFile() {
        return fs_file;
    }

    bool getInUse() {
        return inUse;
    }

    void setInUse(bool n) {
        this -> inUse = n;
    }
};

class fsDisk {
    FILE * sim_disk_fd;
    bool isFormat;
    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //or not. (i.e. if BitVector[0] == 1 , means that the first block is occupied.)
    int * BitVector;
    int BitVectorSize;
    vector < FileDescriptor * > MainDir;
    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    FileDescriptor ** OpenFileDescriptors;
    int direct_enteris;
    int block_size;
    int maxSize;
    int freeBlocks;

public:
    fsDisk() { /// constructor.
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        direct_enteris = 0;
        block_size = 0;
        maxSize = 0;
        isFormat = false;
    }

    ~fsDisk() { /// destructor
        for (int i = 0; i < BitVectorSize; i++) {
            OpenFileDescriptors[i] = nullptr;
        }
        delete[] OpenFileDescriptors;

        for (auto & i: MainDir) {
            delete i -> getFsFile();
            delete i;
        }

        delete[] BitVector;
        fclose(sim_disk_fd);
    }

    void listAll() { /// This method prints the contents of the disk.
        for (size_t i = 0; i < MainDir.size(); i++) {
            cout << "index: " << i << ": FileName: " << MainDir[i] -> getFileName() << " , isInUse: " <<
            MainDir[i] -> getInUse() << endl;
        }
        char buffer;
        cout << "Disk content: '";
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread( & buffer, 1, 1, sim_disk_fd);
            cout << buffer;
        }
        cout << "'" << endl;
    }

    void fsFormat(int blockSize) {
        /// This method gets a number and formats the disk so that the size of each block is the size of that number.
        if (isFormat) {
            for (int i = 0; i < DISK_SIZE; i++) {
                size_t ret_val = fseek(sim_disk_fd, i, SEEK_SET);
                ret_val = fwrite("\0", 1, 1, sim_disk_fd);
                assert(ret_val == 1);
            }
            for (int i = 0; i < BitVectorSize; i++) {
                OpenFileDescriptors[i] = nullptr;
            }
            delete[] OpenFileDescriptors;

            for (auto & i: MainDir) {
                delete i -> getFsFile();
                delete i;
            }
            MainDir.clear();
            delete[] BitVector;
        }
        this -> block_size = blockSize;
        this -> maxSize = blockSize * block_size; /// how many chars you can write to file.
        BitVectorSize = DISK_SIZE / block_size;
        BitVector = new int[BitVectorSize];
        for (int i = 0; i < BitVectorSize; i++) {
            BitVector[i] = 0;
        }
        OpenFileDescriptors = new FileDescriptor * [BitVectorSize];
        for (int i = 0; i < BitVectorSize; i++) {
            OpenFileDescriptors[i] = nullptr;
        }
        this -> freeBlocks = BitVectorSize;
        this -> isFormat = true;
    }

    int CreateFile(string fileName) {
        /// This method creates a new file and allocates disk space to a management block.
        if (!isFormat) {
            return -1;
        }
        FsFile * fs = new FsFile(block_size);
        int available = freeOpen();
        FileDescriptor * fd = new FileDescriptor(fileName, fs);
        MainDir.push_back(fd);
        OpenFileDescriptors[available] = fd;
        return available;
    }

    int OpenFile(string fileName) { /// This method opens a file that was closed.
        if (!isFormat) {
            return -1;
        }
        bool exist = false;
        bool isOpen = false;
        int res = -1;
        int temp;

        for (size_t i = 0; i < MainDir.size(); i++) {
            if (MainDir[i] -> getFileName() == fileName) {
                exist = true;
                temp = i;
                break;
            }
        }
        if (exist) {
            if (MainDir[temp] -> getInUse()) {
                isOpen = true;
            }
        } else {
            //cout << "the file not exist. ";
            return res;
        }
        if (!isOpen) {
            int available = freeOpen();
            MainDir[temp] -> setInUse(true);
            OpenFileDescriptors[available] = MainDir[temp];
            res = available;
        }
        return res;
    }

    string CloseFile(int fd) { /// This method closes a file that was open.
        if (!isFormat || fd >= BitVectorSize) {
            return "-1";
        }
        bool isOpen = false;
        string res = "-1";
        if (OpenFileDescriptors[fd] != nullptr) {
            isOpen = true;
        }
        if (isOpen) {
            res = OpenFileDescriptors[fd] -> getFileName();
            OpenFileDescriptors[fd] -> setInUse(false);
            OpenFileDescriptors[fd] = nullptr;
        } else {
            //cout << "the file is not open. ";
            return "-1";
        }
        return res;
    }

    char decToChar(int n, char & c) { /// This method gets a number and turns it into a character.
        // array to store binary number
        int binaryNum[8];

        // counter for binary array
        int i = 0;
        while (n > 0) {
            // storing remainder in binary array
            binaryNum[i] = n % 2;
            n = n / 2;
            i++;
        }
        // printing binary array in reverse order
        for (int j = i - 1; j >= 0; j--) {
            if (binaryNum[j] == 1)
                c = c | 1u << j;
        }
        return c;
    }

    int WriteToFile(int fd, char * buf, int len) { /// This method writes characters into a selected file.
        if (!isFormat || fd >= BitVectorSize) {
            return -1;
        }
        if (OpenFileDescriptors[fd] == nullptr) { ///the file is close.
            return -1;
        }
        int numOfChars = OpenFileDescriptors[fd] -> getFsFile() -> getNumOfChars();
        if (numOfChars == 0) {
            int freeBlock = -1;
            for (int i = 0; i < BitVectorSize; i++) {
                if (BitVector[i] == 0) {
                    freeBlock = i;
                    break;
                }
            }
            if (freeBlock == -1) {
                //cout << "There is no disk space." << endl;
                return -1;
            }
            OpenFileDescriptors[fd] -> getFsFile() -> setIndexBlock(freeBlock);
            BitVector[freeBlock] = 1;
        }
        if ((maxSize - numOfChars) < len) {
            /// check if we have space in the file.
            return -1;
        }
        int blockInUse = OpenFileDescriptors[fd] -> getFsFile() -> getBlockInUse();
        int sum = numOfChars + len;
        int blockToAllocate = sum / block_size;
        if (sum % block_size != 0) {
            blockToAllocate++;
        }
        blockToAllocate = blockToAllocate - blockInUse;
        int charsInLastBlock = numOfChars % block_size;
        if (charsInLastBlock != 0) {
            charsInLastBlock = block_size - charsInLastBlock;
        }
        // check if we have enough blocks in disk
        int countFree = 0;
        for (int i = 0; i < BitVectorSize; i++) {
            if (countFree == blockToAllocate) {
                break;
            }
            if (BitVector[i] == 0) {
                countFree++;
            }
        }
        if (countFree == blockToAllocate) {
            int cursor;
            int charsWritten = 0;
            int cursorInBuf = 0;
            if (charsInLastBlock != 0) {
                cursor = (OpenFileDescriptors[fd] -> getFsFile() -> getIndexBlock()) * block_size +
                        OpenFileDescriptors[fd] -> getFsFile() -> getBlockInUse() - 1;
                if (fseek(sim_disk_fd, cursor, SEEK_SET) == -1) {
                    cerr << "fseek failed" << endl;
                    return -1;
                }
                char c = 0;
                if (fread( & c, sizeof(char), 1, sim_disk_fd) != 1) {
                    cerr << "fread failed";
                    return -1;
                }
                cursor = (int) c;
                cursor = (cursor * block_size) + (numOfChars % block_size);
                /// write to last block
                int i = 0;
                for (; i < charsInLastBlock; i++) {
                    if (fseek(sim_disk_fd, cursor + i, SEEK_SET) == -1) {
                        cerr << "fseek failed" << endl;
                        return -1;
                    }
                    if (fwrite(buf + cursorInBuf, 1, 1, sim_disk_fd) == (size_t) - 1) {
                        cerr << "fwrite failed" << endl;
                        return -1;
                    }
                    cursorInBuf++;
                    charsWritten++;
                }
                numOfChars += charsInLastBlock;
                if (charsWritten == len) {
                    OpenFileDescriptors[fd] -> getFsFile() -> setNumOfChars(numOfChars);
                    return len;
                }
            }
            while (charsWritten != len) {
                int freeBlock = -1;
                for (int j = 0; j < BitVectorSize; j++) {
                    if (BitVector[j] == 0) {
                        freeBlock = j;
                        BitVector[j] = 1;
                        break;
                    }
                }
                char temp = 0;
                temp = decToChar(freeBlock, temp);
                cursor = ((OpenFileDescriptors[fd] -> getFsFile() -> getIndexBlock()) * block_size) +
                        OpenFileDescriptors[fd] -> getFsFile() -> getBlockInUse();
                if (fseek(sim_disk_fd, cursor, SEEK_SET) == -1) {
                    cerr << "fseek failed" << endl;
                    return -1;
                }
                if (fwrite( & temp, 1, 1, sim_disk_fd) == (size_t) - 1) {
                    cerr << "fwrite failed" << endl;
                    return -1;
                }
                blockInUse++;
                OpenFileDescriptors[fd] -> getFsFile() -> setBlockInUse(blockInUse);
                cursor = freeBlock * block_size;
                int i = 0;
                for (; i < block_size; i++) {
                    if (charsWritten == len) {
                        break;
                    }
                    if (fseek(sim_disk_fd, cursor + i, SEEK_SET) < 0) {
                        cerr << "fseek failed" << endl;
                        return -1;
                    }
                    if (fwrite(buf + cursorInBuf, 1, 1, sim_disk_fd) == (size_t) - 1) {
                        cerr << "fwrite failed" << endl;
                        return -1;
                    }
                    cursorInBuf++;
                    charsWritten++;
                }
            }
            numOfChars += (len - charsInLastBlock);
            OpenFileDescriptors[fd] -> getFsFile() -> setNumOfChars(numOfChars);
        } else {
            //cout << "There is no disk space." << endl;
            return -1;
        }
        return len;
    }

    int DelFile(string FileName) { /// This method deletes a file.
        if (!isFormat) {
            return -1;
        }
        for (int i = 0; i < BitVectorSize; i++) {
            if ((OpenFileDescriptors[i] != nullptr) && (OpenFileDescriptors[i] -> getFileName() == FileName)) {
                OpenFileDescriptors[i] = nullptr;
                break;
            }
        }
        for (size_t i = 0; i < MainDir.size(); i++) {
            if (MainDir[i] -> getFileName() == FileName) {
                int numOfChars = MainDir[i] -> getFsFile() -> getNumOfChars();
                int indexBlock = MainDir[i] -> getFsFile() -> getIndexBlock();
                int blockInUse = MainDir[i] -> getFsFile() -> getBlockInUse();
                int cursor;
                int charsRead = 0;
                if (numOfChars != 0) {
                    for (int j = 0; j < blockInUse; j++) {
                        cursor = (indexBlock * block_size) + j;
                        if (fseek(sim_disk_fd, cursor, SEEK_SET) < 0) {
                            cerr << "fseek failed" << endl;
                            return -1;
                        }
                        char c;
                        if (fread( & c, sizeof(char), 1, sim_disk_fd) != 1) {
                            cerr << "fread failed";
                            return -1;
                        }
                        cursor = (int) c;
                        BitVector[cursor] = 0;
                        cursor = cursor * block_size;
                        int k = 0;
                        for (; k < block_size; k++) {
                            if (charsRead == numOfChars) {
                                break;
                            }
                            if (fseek(sim_disk_fd, cursor, SEEK_SET) < 0) {
                                cerr << "fseek failed" << endl;
                                return -1;
                            }
                            if (fwrite("\0", 1, 1, sim_disk_fd) == (size_t) - 1) {
                                cerr << "fwrite failed" << endl;
                                return -1;
                            }
                            charsRead++;
                            cursor++;
                        }
                        if (charsRead == numOfChars) {
                            break;
                        }
                    }
                }
                for (int j = 0; j < blockInUse; j++) {
                    cursor = (indexBlock * block_size) + j;
                    if (fseek(sim_disk_fd, cursor, SEEK_SET) < 0) {
                        cerr << "fseek failed" << endl;
                        return -1;
                    }
                    if (fwrite("\0", 1, 1, sim_disk_fd) == (size_t) - 1) {
                        cerr << "fwrite failed" << endl;
                        return -1;
                    }
                }
                BitVector[indexBlock] = 0;
                delete MainDir[i] -> getFsFile();
                delete MainDir[i];
                MainDir.erase(MainDir.begin() + (int) i);
                return 1;
            }
        }
        return -1;
    }

    int ReadFromFile(int fd, char * buf, int len) { /// This method reads the contents of the selected file.
        if (!isFormat || fd >= BitVectorSize) {
            return -1;
        }
        if (OpenFileDescriptors[fd] == nullptr) { /// The file is close.
            return -1;
        }
        int numOfChars = OpenFileDescriptors[fd] -> getFsFile() -> getNumOfChars();
        if (len > numOfChars) {
            buf[0] = '\0';
            return -1;
        }
        int indexBlock = OpenFileDescriptors[fd] -> getFsFile() -> getIndexBlock();
        int blockInUse = OpenFileDescriptors[fd] -> getFsFile() -> getBlockInUse();
        int cursor;
        int cursorInBuf = 0;
        int charsRead = 0;
        for (int i = 0; i < blockInUse; i++) {
            cursor = (indexBlock * block_size) + i;
            if (fseek(sim_disk_fd, cursor, SEEK_SET) < 0) {
                cerr << "fseek failed" << endl;
                return -1;
            }
            char c;
            if (fread( & c, sizeof(char), 1, sim_disk_fd) != 1) {
                cerr << "fread failed";
                return -1;
            }
            cursor = (int) c;
            cursor = cursor * block_size;
            int j = 0;
            for (; j < block_size; j++) {
                if (charsRead == len) {
                    break;
                }
                if (fseek(sim_disk_fd, cursor, SEEK_SET) < 0) {
                    cerr << "fseek failed" << endl;
                    return -1;
                }
                if (fread( & buf[cursorInBuf], sizeof(char), 1, sim_disk_fd) != 1) {
                    cerr << "fread failed" << endl;
                    return -1;
                }
                charsRead++;
                cursor++;
                cursorInBuf++;
            }
            if (charsRead == len) {
                buf[cursorInBuf] = '\0';
                break;
            }
        }
        return len;
    }

    int freeOpen() {
        /// This method checks where there is free space in the array that saves the open files.
        int whoFree = -1;
        for (int i = 0; i < BitVectorSize; i++) {
            if (OpenFileDescriptors[i] == nullptr) {
                whoFree = i;
                break;
            }
        }
        if (whoFree == -1) {
            //cout << "There is not enough space to open more files." << endl;
            return -1;
        }
        return whoFree;
    }
};

/// main
#include <string.h>
#define DISK_SIZE 256

int main() {
    int blockSize;
    //int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;
    int written;

    fsDisk * fs = new fsDisk();
    int cmd_;
    while (1) {
        cin >> cmd_;
        switch (cmd_) {
            case 0: // exit
            delete fs;
            exit(0);
            break;

            case 1: // list-file
            fs -> listAll();
            break;

            case 2: // format
            cin >> blockSize;
            fs -> fsFormat(blockSize);
            break;

            case 3: // creat-file
            cin >> fileName;
            _fd = fs -> CreateFile(fileName);
            cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 4: // open-file
            cin >> fileName;
            _fd = fs -> OpenFile(fileName);
            cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 5: // close-file
            cin >> _fd;
            fileName = fs -> CloseFile(_fd);
            cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 6: // write-file
            cin >> _fd;
            cin >> str_to_write;
            written = fs -> WriteToFile(_fd, str_to_write, strlen(str_to_write));
            cout << "Writed: " << written << " Char's into File Descriptor #: " << _fd << endl;
            break;

            case 7: // read-file
            cin >> _fd;
            cin >> size_to_read;
            fs -> ReadFromFile(_fd, str_to_read, size_to_read);
            cout << "ReadFromFile: " << str_to_read << endl;
            break;

            case 8: // delete file
            cin >> fileName;
            _fd = fs -> DelFile(fileName);
            cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;
            default:
                break;
        }
    }
}