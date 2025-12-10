#pragma once

#include "kernel/types.hpp"
#include "utils/logger.hpp"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace MiniOS {

enum class FileType {
    Regular,
    Directory,
    Device
};

enum class OpenMode {
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Append = 4,
    Create = 8,
    Truncate = 16
};

inline OpenMode operator|(OpenMode a, OpenMode b) {
    return static_cast<OpenMode>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool hasFlag(OpenMode mode, OpenMode flag) {
    return (static_cast<int>(mode) & static_cast<int>(flag)) != 0;
}

struct INode {
    uint32_t inodeNumber;
    FileType type;
    std::string name;
    size_t size;
    std::vector<uint8_t> data;
    
    uint32_t parentInode;
    std::vector<uint32_t> childInodes;
    
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point modificationTime;
    std::chrono::system_clock::time_point accessTime;
    
    MemoryProtection permissions;
    TaskId owner;

    INode(uint32_t inode, FileType t, const std::string& n)
        : inodeNumber(inode)
        , type(t)
        , name(n)
        , size(0)
        , parentInode(0)
        , creationTime(std::chrono::system_clock::now())
        , modificationTime(creationTime)
        , accessTime(creationTime)
        , permissions(MemoryProtection::ReadWrite)
        , owner(0)
    {}
};

struct FileDescriptorEntry {
    uint32_t inodeNumber;
    size_t position;
    OpenMode mode;
    TaskId owner;
    bool isOpen;

    FileDescriptorEntry(uint32_t inode, OpenMode m, TaskId task)
        : inodeNumber(inode)
        , position(0)
        , mode(m)
        , owner(task)
        , isOpen(true)
    {}
};

class FileSystem {
public:
    FileSystem();
    ~FileSystem() = default;

    bool createFile(const std::string& path, TaskId owner);
    bool createDirectory(const std::string& path, TaskId owner);
    bool deleteFile(const std::string& path);
    bool deleteDirectory(const std::string& path);

    FileDescriptor open(const std::string& path, OpenMode mode, TaskId taskId);
    bool close(FileDescriptor fd);

    ssize_t read(FileDescriptor fd, void* buffer, size_t count);
    ssize_t write(FileDescriptor fd, const void* buffer, size_t count);
    bool seek(FileDescriptor fd, size_t position);

    bool exists(const std::string& path) const;
    std::optional<FileType> getType(const std::string& path) const;
    std::optional<size_t> getSize(const std::string& path) const;

    std::vector<std::string> listDirectory(const std::string& path) const;
    std::string getCurrentDirectory() const { return currentDirectory_; }
    bool changeDirectory(const std::string& path);

    std::string getFileSystemReport() const;
    void printDirectoryTree(const std::string& path = "/", int indent = 0) const;

private:
    std::vector<std::string> parsePath(const std::string& path) const;
    std::string normalizePath(const std::string& path) const;
    INode* findINode(const std::string& path);
    const INode* findINode(const std::string& path) const;
    INode* getParentDirectory(const std::string& path);
    std::string getFileName(const std::string& path) const;

    std::map<uint32_t, std::unique_ptr<INode>> inodes_;
    std::map<FileDescriptor, FileDescriptorEntry> fdTable_;
    
    uint32_t nextInodeNumber_;
    FileDescriptor nextFd_;
    std::string currentDirectory_;
    
    static constexpr uint32_t ROOT_INODE = 1;
};

}
