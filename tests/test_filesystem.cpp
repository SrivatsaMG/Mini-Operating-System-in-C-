#include "fs/filesystem.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace MiniOS;

void test_file_creation() {
    std::cout << "Testing file creation... ";
    
    FileSystem fs;
    
    bool result = fs.createFile("/test.txt", 0);
    assert(result == true);
    assert(fs.exists("/test.txt") == true);
    
    result = fs.createFile("/test.txt", 0);
    assert(result == false);
    
    std::cout << "PASSED\n";
}

void test_directory_creation() {
    std::cout << "Testing directory creation... ";
    
    FileSystem fs;
    
    bool result = fs.createDirectory("/home", 0);
    assert(result == true);
    
    result = fs.createDirectory("/home/user", 0);
    assert(result == true);
    
    assert(fs.exists("/home") == true);
    assert(fs.exists("/home/user") == true);
    
    auto type = fs.getType("/home");
    assert(type.has_value());
    assert(*type == FileType::Directory);
    
    std::cout << "PASSED\n";
}

void test_file_read_write() {
    std::cout << "Testing file read/write... ";
    
    FileSystem fs;
    
    auto fd = fs.open("/data.txt", OpenMode::ReadWrite | OpenMode::Create, 0);
    assert(fd != INVALID_FD);
    
    const char* testData = "Hello, MiniOS!";
    ssize_t written = fs.write(fd, testData, strlen(testData));
    assert(written == static_cast<ssize_t>(strlen(testData)));
    
    fs.seek(fd, 0);
    
    char buffer[256] = {0};
    ssize_t bytesRead = fs.read(fd, buffer, sizeof(buffer));
    assert(bytesRead == static_cast<ssize_t>(strlen(testData)));
    assert(strcmp(buffer, testData) == 0);
    
    fs.close(fd);
    
    std::cout << "PASSED\n";
}

void test_file_deletion() {
    std::cout << "Testing file deletion... ";
    
    FileSystem fs;
    
    fs.createFile("/temp.txt", 0);
    assert(fs.exists("/temp.txt") == true);
    
    bool result = fs.deleteFile("/temp.txt");
    assert(result == true);
    assert(fs.exists("/temp.txt") == false);
    
    std::cout << "PASSED\n";
}

void test_directory_listing() {
    std::cout << "Testing directory listing... ";
    
    FileSystem fs;
    
    fs.createDirectory("/docs", 0);
    fs.createFile("/docs/file1.txt", 0);
    fs.createFile("/docs/file2.txt", 0);
    fs.createDirectory("/docs/subdir", 0);
    
    auto contents = fs.listDirectory("/docs");
    assert(contents.size() == 3);
    
    std::cout << "PASSED\n";
}

void test_path_normalization() {
    std::cout << "Testing path normalization... ";
    
    FileSystem fs;
    
    fs.createDirectory("/a", 0);
    fs.createDirectory("/a/b", 0);
    fs.createFile("/a/b/test.txt", 0);
    
    assert(fs.exists("/a/b/test.txt") == true);
    assert(fs.exists("/a/b/../b/test.txt") == true);
    assert(fs.exists("/a/./b/./test.txt") == true);
    
    std::cout << "PASSED\n";
}

void test_file_descriptor_operations() {
    std::cout << "Testing file descriptor operations... ";
    
    FileSystem fs;
    
    fs.createFile("/fdtest.txt", 0);
    
    auto fd1 = fs.open("/fdtest.txt", OpenMode::ReadWrite, 0);
    auto fd2 = fs.open("/fdtest.txt", OpenMode::Read, 0);
    
    assert(fd1 != INVALID_FD);
    assert(fd2 != INVALID_FD);
    assert(fd1 != fd2);
    
    fs.close(fd1);
    fs.close(fd2);
    
    std::cout << "PASSED\n";
}

int main() {
    Logger::instance().setLevel(LogLevel::Error);
    
    std::cout << "\n=== File System Unit Tests ===\n\n";
    
    test_file_creation();
    test_directory_creation();
    test_file_read_write();
    test_file_deletion();
    test_directory_listing();
    test_path_normalization();
    test_file_descriptor_operations();
    
    std::cout << "\nAll file system tests passed!\n\n";
    return 0;
}
