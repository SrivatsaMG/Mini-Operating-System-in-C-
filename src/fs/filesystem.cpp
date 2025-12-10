#include "fs/filesystem.hpp"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstring>

namespace MiniOS {

FileSystem::FileSystem()
    : nextInodeNumber_(ROOT_INODE + 1)
    , nextFd_(0)
    , currentDirectory_("/")
{
    auto root = std::make_unique<INode>(ROOT_INODE, FileType::Directory, "/");
    root->parentInode = ROOT_INODE;
    inodes_[ROOT_INODE] = std::move(root);
    
    LOG_INFO("FileSystem", "Initialized in-memory file system");
}

bool FileSystem::createFile(const std::string& path, TaskId owner) {
    std::string normalPath = normalizePath(path);
    
    if (exists(normalPath)) {
        LOG_WARN("FileSystem", "File already exists: " + normalPath);
        return false;
    }
    
    INode* parent = getParentDirectory(normalPath);
    if (!parent || parent->type != FileType::Directory) {
        LOG_ERROR("FileSystem", "Parent directory not found for: " + normalPath);
        return false;
    }
    
    uint32_t inode = nextInodeNumber_++;
    std::string fileName = getFileName(normalPath);
    
    auto file = std::make_unique<INode>(inode, FileType::Regular, fileName);
    file->parentInode = parent->inodeNumber;
    file->owner = owner;
    
    parent->childInodes.push_back(inode);
    inodes_[inode] = std::move(file);
    
    LOG_INFO("FileSystem", "Created file: " + normalPath);
    return true;
}

bool FileSystem::createDirectory(const std::string& path, TaskId owner) {
    std::string normalPath = normalizePath(path);
    
    if (exists(normalPath)) {
        LOG_WARN("FileSystem", "Directory already exists: " + normalPath);
        return false;
    }
    
    INode* parent = getParentDirectory(normalPath);
    if (!parent || parent->type != FileType::Directory) {
        LOG_ERROR("FileSystem", "Parent directory not found for: " + normalPath);
        return false;
    }
    
    uint32_t inode = nextInodeNumber_++;
    std::string dirName = getFileName(normalPath);
    
    auto dir = std::make_unique<INode>(inode, FileType::Directory, dirName);
    dir->parentInode = parent->inodeNumber;
    dir->owner = owner;
    
    parent->childInodes.push_back(inode);
    inodes_[inode] = std::move(dir);
    
    LOG_INFO("FileSystem", "Created directory: " + normalPath);
    return true;
}

bool FileSystem::deleteFile(const std::string& path) {
    std::string normalPath = normalizePath(path);
    INode* file = findINode(normalPath);
    
    if (!file) {
        LOG_ERROR("FileSystem", "File not found: " + normalPath);
        return false;
    }
    
    if (file->type != FileType::Regular) {
        LOG_ERROR("FileSystem", "Not a regular file: " + normalPath);
        return false;
    }
    
    INode* parent = nullptr;
    auto parentIt = inodes_.find(file->parentInode);
    if (parentIt != inodes_.end()) {
        parent = parentIt->second.get();
    }
    
    if (parent) {
        auto& children = parent->childInodes;
        children.erase(std::remove(children.begin(), children.end(), file->inodeNumber), children.end());
    }
    
    inodes_.erase(file->inodeNumber);
    LOG_INFO("FileSystem", "Deleted file: " + normalPath);
    return true;
}

bool FileSystem::deleteDirectory(const std::string& path) {
    std::string normalPath = normalizePath(path);
    INode* dir = findINode(normalPath);
    
    if (!dir) {
        LOG_ERROR("FileSystem", "Directory not found: " + normalPath);
        return false;
    }
    
    if (dir->type != FileType::Directory) {
        LOG_ERROR("FileSystem", "Not a directory: " + normalPath);
        return false;
    }
    
    if (!dir->childInodes.empty()) {
        LOG_ERROR("FileSystem", "Directory not empty: " + normalPath);
        return false;
    }
    
    if (dir->inodeNumber == ROOT_INODE) {
        LOG_ERROR("FileSystem", "Cannot delete root directory");
        return false;
    }
    
    INode* parent = nullptr;
    auto parentIt = inodes_.find(dir->parentInode);
    if (parentIt != inodes_.end()) {
        parent = parentIt->second.get();
    }
    
    if (parent) {
        auto& children = parent->childInodes;
        children.erase(std::remove(children.begin(), children.end(), dir->inodeNumber), children.end());
    }
    
    inodes_.erase(dir->inodeNumber);
    LOG_INFO("FileSystem", "Deleted directory: " + normalPath);
    return true;
}

FileDescriptor FileSystem::open(const std::string& path, OpenMode mode, TaskId taskId) {
    std::string normalPath = normalizePath(path);
    
    if (hasFlag(mode, OpenMode::Create) && !exists(normalPath)) {
        if (!createFile(normalPath, taskId)) {
            return INVALID_FD;
        }
    }
    
    INode* file = findINode(normalPath);
    if (!file) {
        LOG_ERROR("FileSystem", "Cannot open: " + normalPath);
        return INVALID_FD;
    }
    
    if (file->type != FileType::Regular) {
        LOG_ERROR("FileSystem", "Cannot open directory as file: " + normalPath);
        return INVALID_FD;
    }
    
    if (hasFlag(mode, OpenMode::Truncate)) {
        file->data.clear();
        file->size = 0;
    }
    
    FileDescriptor fd = nextFd_++;
    fdTable_.emplace(fd, FileDescriptorEntry(file->inodeNumber, mode, taskId));
    
    if (hasFlag(mode, OpenMode::Append)) {
        fdTable_.at(fd).position = file->size;
    }
    
    LOG_DEBUG("FileSystem", "Opened file: " + normalPath + " (fd=" + std::to_string(fd) + ")");
    return fd;
}

bool FileSystem::close(FileDescriptor fd) {
    auto it = fdTable_.find(fd);
    if (it == fdTable_.end()) {
        return false;
    }
    
    fdTable_.erase(it);
    LOG_DEBUG("FileSystem", "Closed fd " + std::to_string(fd));
    return true;
}

ssize_t FileSystem::read(FileDescriptor fd, void* buffer, size_t count) {
    auto fdIt = fdTable_.find(fd);
    if (fdIt == fdTable_.end() || !fdIt->second.isOpen) {
        return -1;
    }
    
    auto& fdEntry = fdIt->second;
    
    if (!hasFlag(fdEntry.mode, OpenMode::Read) && !hasFlag(fdEntry.mode, OpenMode::ReadWrite)) {
        LOG_ERROR("FileSystem", "File not opened for reading");
        return -1;
    }
    
    auto inodeIt = inodes_.find(fdEntry.inodeNumber);
    if (inodeIt == inodes_.end()) {
        return -1;
    }
    
    INode* file = inodeIt->second.get();
    file->accessTime = std::chrono::system_clock::now();
    
    size_t available = file->size - fdEntry.position;
    size_t toRead = std::min(count, available);
    
    if (toRead > 0) {
        std::memcpy(buffer, file->data.data() + fdEntry.position, toRead);
        fdEntry.position += toRead;
    }
    
    return static_cast<ssize_t>(toRead);
}

ssize_t FileSystem::write(FileDescriptor fd, const void* buffer, size_t count) {
    auto fdIt = fdTable_.find(fd);
    if (fdIt == fdTable_.end() || !fdIt->second.isOpen) {
        return -1;
    }
    
    auto& fdEntry = fdIt->second;
    
    if (!hasFlag(fdEntry.mode, OpenMode::Write) && !hasFlag(fdEntry.mode, OpenMode::ReadWrite)) {
        LOG_ERROR("FileSystem", "File not opened for writing");
        return -1;
    }
    
    auto inodeIt = inodes_.find(fdEntry.inodeNumber);
    if (inodeIt == inodes_.end()) {
        return -1;
    }
    
    INode* file = inodeIt->second.get();
    
    size_t newSize = fdEntry.position + count;
    if (newSize > file->data.size()) {
        file->data.resize(newSize);
    }
    
    std::memcpy(file->data.data() + fdEntry.position, buffer, count);
    fdEntry.position += count;
    file->size = std::max(file->size, newSize);
    file->modificationTime = std::chrono::system_clock::now();
    
    return static_cast<ssize_t>(count);
}

bool FileSystem::seek(FileDescriptor fd, size_t position) {
    auto fdIt = fdTable_.find(fd);
    if (fdIt == fdTable_.end()) {
        return false;
    }
    
    fdIt->second.position = position;
    return true;
}

bool FileSystem::exists(const std::string& path) const {
    return findINode(path) != nullptr;
}

std::optional<FileType> FileSystem::getType(const std::string& path) const {
    const INode* inode = findINode(path);
    if (!inode) return std::nullopt;
    return inode->type;
}

std::optional<size_t> FileSystem::getSize(const std::string& path) const {
    const INode* inode = findINode(path);
    if (!inode) return std::nullopt;
    return inode->size;
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path) const {
    std::vector<std::string> result;
    const INode* dir = findINode(path);
    
    if (!dir || dir->type != FileType::Directory) {
        return result;
    }
    
    for (uint32_t childInode : dir->childInodes) {
        auto it = inodes_.find(childInode);
        if (it != inodes_.end()) {
            result.push_back(it->second->name);
        }
    }
    
    return result;
}

bool FileSystem::changeDirectory(const std::string& path) {
    std::string normalPath = normalizePath(path);
    const INode* dir = findINode(normalPath);
    
    if (!dir || dir->type != FileType::Directory) {
        return false;
    }
    
    currentDirectory_ = normalPath;
    return true;
}

std::string FileSystem::getFileSystemReport() const {
    std::stringstream ss;
    ss << "=== File System Report ===\n";
    ss << "Total Inodes: " << inodes_.size() << "\n";
    ss << "Open File Descriptors: " << fdTable_.size() << "\n";
    ss << "Current Directory: " << currentDirectory_ << "\n";
    
    size_t totalSize = 0;
    size_t fileCount = 0;
    size_t dirCount = 0;
    
    for (const auto& [_, inode] : inodes_) {
        if (inode->type == FileType::Regular) {
            fileCount++;
            totalSize += inode->size;
        } else if (inode->type == FileType::Directory) {
            dirCount++;
        }
    }
    
    ss << "Files: " << fileCount << "\n";
    ss << "Directories: " << dirCount << "\n";
    ss << "Total Data Size: " << totalSize << " bytes\n";
    
    return ss.str();
}

void FileSystem::printDirectoryTree(const std::string& path, int indent) const {
    const INode* dir = findINode(path);
    if (!dir) return;
    
    std::cout << std::string(indent, ' ') << dir->name;
    if (dir->type == FileType::Directory) {
        std::cout << "/";
    }
    std::cout << std::endl;
    
    if (dir->type == FileType::Directory) {
        for (uint32_t childInode : dir->childInodes) {
            auto it = inodes_.find(childInode);
            if (it != inodes_.end()) {
                std::string childPath = (path == "/" ? "/" : path + "/") + it->second->name;
                printDirectoryTree(childPath, indent + 2);
            }
        }
    }
}

std::vector<std::string> FileSystem::parsePath(const std::string& path) const {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '/')) {
        if (!part.empty() && part != ".") {
            if (part == "..") {
                if (!parts.empty()) {
                    parts.pop_back();
                }
            } else {
                parts.push_back(part);
            }
        }
    }
    
    return parts;
}

std::string FileSystem::normalizePath(const std::string& path) const {
    std::string fullPath = path;
    
    if (path.empty() || path[0] != '/') {
        fullPath = currentDirectory_;
        if (fullPath.back() != '/') {
            fullPath += '/';
        }
        fullPath += path;
    }
    
    auto parts = parsePath(fullPath);
    if (parts.empty()) {
        return "/";
    }
    
    std::string result = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        result += parts[i];
        if (i < parts.size() - 1) {
            result += "/";
        }
    }
    
    return result;
}

INode* FileSystem::findINode(const std::string& path) {
    return const_cast<INode*>(static_cast<const FileSystem*>(this)->findINode(path));
}

const INode* FileSystem::findINode(const std::string& path) const {
    std::string normalPath = normalizePath(path);
    
    if (normalPath == "/") {
        auto it = inodes_.find(ROOT_INODE);
        return it != inodes_.end() ? it->second.get() : nullptr;
    }
    
    auto parts = parsePath(normalPath);
    
    auto currentIt = inodes_.find(ROOT_INODE);
    if (currentIt == inodes_.end()) {
        return nullptr;
    }
    
    const INode* current = currentIt->second.get();
    
    for (const auto& part : parts) {
        if (current->type != FileType::Directory) {
            return nullptr;
        }
        
        bool found = false;
        for (uint32_t childInode : current->childInodes) {
            auto childIt = inodes_.find(childInode);
            if (childIt != inodes_.end() && childIt->second->name == part) {
                current = childIt->second.get();
                found = true;
                break;
            }
        }
        
        if (!found) {
            return nullptr;
        }
    }
    
    return current;
}

INode* FileSystem::getParentDirectory(const std::string& path) {
    std::string normalPath = normalizePath(path);
    
    size_t lastSlash = normalPath.rfind('/');
    if (lastSlash == std::string::npos || lastSlash == 0) {
        auto it = inodes_.find(ROOT_INODE);
        return it != inodes_.end() ? it->second.get() : nullptr;
    }
    
    std::string parentPath = normalPath.substr(0, lastSlash);
    if (parentPath.empty()) {
        parentPath = "/";
    }
    
    return findINode(parentPath);
}

std::string FileSystem::getFileName(const std::string& path) const {
    size_t lastSlash = path.rfind('/');
    if (lastSlash == std::string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

}
