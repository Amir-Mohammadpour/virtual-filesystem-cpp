#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <fstream>

using namespace std;

const int SECTOR_SIZE = 64;

class FileSystem
{
private:
    class Item
    {
    public:
        bool isFolder;
        string name;
        string content;
        vector<int> sectors;
        vector<Item *> children;
        Item *parent;
    };
    vector<string> disk;
    vector<bool> sectorMap;
    Item *root;
    Item *currentDir;
    int totalSectors;

    int allocateSector()
    {
        for (int i = 0; i < sectorMap.size(); i++)
        {
            if (!sectorMap[i])
            {
                sectorMap[i] = true;
                return i;
            }
        }
        throw runtime_error("No free sectors available");
    }

    void freeSector(int sector)
    {
        if (sector < 0 || sector >= sectorMap.size())
        {
            throw out_of_range("Invalid sector number: " + to_string(sector) +
                               ". Valid range: 0 to " + to_string(sectorMap.size() - 1));
        }
        sectorMap[sector] = false;
    }

    void saveToDisk(Item *file)
    {
        if (!file || file->isFolder)
            return;

        for (int sector : file->sectors)
            freeSector(sector);
        file->sectors.clear();

        string data = file->content;
        int pos = 0;
        while (pos < data.length())
        {
            int sector = allocateSector();
            file->sectors.push_back(sector);

            while (sector >= disk.size())
                disk.push_back("");

            int len = min(SECTOR_SIZE, (int)data.length() - pos);
            disk[sector] = data.substr(pos, len);
            pos += len;
        }
    }

    vector<string> splitPath(const string &path)
    {
        vector<string> parts;
        string part;
        istringstream stream(path);

        while (getline(stream, part, '/'))
        {
            if (!part.empty())
                parts.push_back(part);
        }
        return parts;
    }

    Item *getItem(const string &path)
    {
        if (path.empty() || path == ".")
            return currentDir;
        if (path == "/")
            return root;
        if (path == "..")
            return (currentDir->parent) ? currentDir->parent : currentDir;

        Item *current = currentDir;
        vector<string> parts;
        if (path[0] == '/')
        {
            current = root;
            parts = splitPath(path);
        }
        else
            parts = splitPath(path);

        for (const string &part : parts)
        {
            if (part == ".")
                continue;
            if (part == "..")
            {
                if (current->parent)
                    current = current->parent;
                continue;
            }

            bool found = false;
            for (Item *child : current->children)
            {
                if (child->name == part)
                {
                    current = child;
                    found = true;
                    break;
                }
            }
            if (!found)
                return nullptr;
        }
        return current;
    }

    void collectAllFiles(Item *folder, vector<Item *> &files)
    {
        for (Item *child : folder->children)
        {
            if (child->isFolder)
                collectAllFiles(child, files);
            else
                files.push_back(child);
        }
    }

    bool isValidName(const string &name)
    {
        if (name.empty() || name == "." || name == "..")
            return false;
        for (char c : name)
        {
            if (!isalnum(c) && c != '_' && c != '.')
                return false;
        }
        return true;
    }

    string getFullPath(Item *item)
    {
        if (item == root)
            return "/";

        vector<string> pathParts;

        for (Item *current = item; current != root; current = current->parent)
            pathParts.push_back(current->name);

        reverse(pathParts.begin(), pathParts.end());

        string fullPath = "/";
        for (int i = 0; i < pathParts.size(); ++i)
        {
            if (i > 0)
                fullPath += "/";
            fullPath += pathParts[i];
        }

        return fullPath;
    }

    void freeSectorsRecursive(Item *item)
    {
        if (!item)
            return;

        if (!item->isFolder)
        {
            for (int sector : item->sectors)
                freeSector(sector);
        }

        for (Item *child : item->children)
            freeSectorsRecursive(child);
    }

    void deleteTree(Item *node)
    {
        if (!node)
            return;

        for (Item *child : node->children)
            deleteTree(child);

        freeSectorsRecursive(node);
        delete node;
    }

    Item *copyItem(Item *source, Item *newParent)
    {
        Item *newItem = new Item();
        newItem->isFolder = source->isFolder;
        newItem->name = source->name;
        newItem->content = source->content;
        newItem->parent = newParent;

        if (!source->isFolder)
        {
            newItem->sectors.clear();
            saveToDisk(newItem);
        }

        for (Item *child : source->children)
        {
            Item *newChild = copyItem(child, newItem);
            newItem->children.push_back(newChild);
        }

        return newItem;
    }

public:
    FileSystem(int capacity) : totalSectors(capacity)
    {
        sectorMap.resize(totalSectors, false);
        disk.resize(totalSectors, "");

        root = new Item();
        root->isFolder = true;
        root->name = "/";
        root->parent = nullptr;

        currentDir = root;
    }

    ~FileSystem()
    {
        deleteTree(root);
    }

    void pwd()
    {
        cout << getFullPath(currentDir) << endl;
    }

    void cd(const string &path)
    {
        try
        {
            Item *target = getItem(path);

            if (!target)
                throw runtime_error("Directory not found: " + path);
            if (!target->isFolder)
                throw runtime_error("Not a directory: " + path);

            currentDir = target;
        }
        catch (const exception &e)
        {
            cerr << e.what() << '\n';
        }

    }

    void ls(const string &path = "")
    {
        try
        {
            Item *target = currentDir;
            if (!path.empty())
            {
                target = getItem(path);
                if (!target)
                    throw runtime_error("Path not found: " + path);

                if (!target->isFolder)
                {
                    cout << "Name: " << target->name << endl;
                    cout << "Path: " << getFullPath(target) << endl;
                    cout << "Size: " << target->content.length() << " bytes" << endl;
                    return;
                }
            }

            vector<string> items;
            for (Item *child : target->children)
            {
                string name = child->name;
                if (child->isFolder)
                    name += "/";
                items.push_back(name);
            }

            sort(items.begin(), items.end());

            for (const string &item : items)
                cout << item << endl;
        }
        catch (const exception &e)
        {
            cerr << e.what() << '\n';
        }
    }

    void mkdir(const string &path)
    {
        try
        {
            if (path.empty())
                throw runtime_error("mkdir: missing path");

            vector<string> parts = splitPath(path);
            if (parts.empty())
                throw runtime_error("Invalid path");

            Item *current = (path[0] == '/') ? root : currentDir;

            for (const string &part : parts)
            {
                if (part.empty() || part == "." || part == "..")
                    throw runtime_error("Invalid directory name in path: " + part);

                if (!isValidName(part))
                    throw runtime_error("Invalid directory name: " + part);

                bool found = false;
                Item *next = nullptr;
                for (Item *child : current->children)
                {
                    if (child->name == part)
                    {
                        if (!child->isFolder)
                            throw runtime_error("Cannot create directory: '" + part + "' — a file with this name exists");
                        next = child;
                        found = true;
                        break;
                    }
                }

                if (found)
                    current = next;
                else
                {
                    Item *newDir = new Item();
                    newDir->isFolder = true;
                    newDir->name = part;
                    newDir->parent = current;
                    newDir->children = {};

                    current->children.push_back(newDir);
                    current = newDir;

                    cout << "Directory created:" << getFullPath(current) << endl;
                }
            }
        }
        catch (const exception &e)
        {
            cerr << "Error:" << e.what() << endl;
        }
    }

    void touch(const string &filename)
    {
        if (!isValidName(filename))
            throw runtime_error("Invalid file name: " + filename);
        for (Item *child : currentDir->children)
        {
            if (child->name == filename)
                throw runtime_error("File already exists: " + filename);
        }

        Item *newFile = new Item();
        newFile->isFolder = false;
        newFile->name = filename;
        newFile->content = "";
        newFile->parent = currentDir;

        currentDir->children.push_back(newFile);
        saveToDisk(newFile);
        cout << "File created: " << filename << endl;
    }

    void rm(const string &name, bool recursive = false)
    {
        try
        {
            Item *target = nullptr;
            for (Item *child : currentDir->children)
            {
                if (name == child->name)
                {
                    target = child;
                    break;
                }
            }

            if (!target)
                throw runtime_error("File or directory not found: " + name);
            if (target->isFolder && !target->children.empty() && !recursive)
                throw runtime_error("Directory is not empty. Use -r flag to remove recursively");

            auto &folders = currentDir->children;
            auto it = find(folders.begin(), folders.end(), target);
            if (it != folders.end())
                folders.erase(it);

            deleteTree(target);

            cout << "Removed: " << name;
            if (recursive)
                cout << " (recursively)";
            cout << endl;
        }
        catch (const exception &e)
        {
            cerr << e.what() << '\n';
        }
    }

    void cp(const string &source, const string &dest)
    {
        try
        {
            Item *srcItem = getItem(source);
            if (!srcItem)
                throw runtime_error("Source not found: " + source);

            Item *destItem = getItem(dest);

            if (destItem && destItem->isFolder)
            {
                string destName = srcItem->name;
                for (Item *child : destItem->children)
                {
                    if (child->name == destName)
                        throw runtime_error("Destination already exists: " + destName);
                }

                Item *newItem = copyItem(srcItem, destItem);
                destItem->children.push_back(newItem);

                cout << "Copied: " << source << " -> " << dest << "/" << destName << endl;
            }

            else
            {
                int lastSlash = dest.find_last_of('/');
                string destDirPath, destName;

                if (lastSlash == string::npos)
                {
                    destDirPath = ".";
                    destName = dest;
                }
                else
                {
                    destDirPath = dest.substr(0, lastSlash);
                    destName = dest.substr(lastSlash + 1);
                }

                Item *destDir = getItem(destDirPath);
                if (!destDir || !destDir->isFolder)
                    throw runtime_error("Destination directory not found");

                for (Item *child : destDir->children)
                {
                    if (child->name == destName)
                        throw runtime_error("Destination already exists: " + destName);
                }

                Item *newItem = copyItem(srcItem, destDir);
                newItem->name = destName;
                destDir->children.push_back(newItem);

                cout << "Copied: " << source << " -> " << dest << endl;
            }
        }
        catch (const std::exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void mv(const string &source, const string &dest)
    {
        try
        {
            Item *srcItem = getItem(source);
            if (!srcItem)
                throw runtime_error("Source not found: " + source);

            if (srcItem->isFolder)
            {
                Item *checkItem = getItem(dest);
                while (checkItem)
                {
                    if (checkItem == srcItem)
                    {
                        throw runtime_error("Cannot move a folder into itself");
                    }
                    checkItem = checkItem->parent;
                }
            }

            Item *destItem = getItem(dest);

            if (destItem && destItem->isFolder)
            {
                for (Item *child : destItem->children)
                {
                    if (child->name == srcItem->name)
                        throw runtime_error("Destination already exists: " + srcItem->name);
                }

                if (srcItem->parent != destItem)
                {
                    if (srcItem->parent)
                    {
                        auto &folder = srcItem->parent->children;
                        auto it = find(folder.begin(), folder.end(), srcItem);
                        if (it != folder.end())
                            folder.erase(it);
                    }

                    srcItem->parent = destItem;
                    destItem->children.push_back(srcItem);
                }

                cout << "Moved: " << source << " -> " << dest << "/" << srcItem->name << endl;
            }

            else
            {

                int lastSlash = dest.find_last_of('/');
                string destDirPath, destName;

                if (lastSlash == string::npos)
                {
                    destDirPath = ".";
                    destName = dest;
                }
                else
                {
                    destDirPath = dest.substr(0, lastSlash);
                    destName = dest.substr(lastSlash + 1);
                }

                Item *destDir = getItem(destDirPath);
                if (!destDir || !destDir->isFolder)
                    throw runtime_error("Destination directory not found");

                for (Item *child : destDir->children)
                {
                    if (child->name == destName && child != srcItem)
                        throw runtime_error("Destination already exists: " + destName);
                }

                bool sameParent = (srcItem->parent == destDir);

                if (!sameParent)
                {
                    if (srcItem->parent)
                    {
                        auto &siblings = srcItem->parent->children;
                        auto it = find(siblings.begin(), siblings.end(), srcItem);
                        if (it != siblings.end())
                            siblings.erase(it);
                    }
                }

                srcItem->name = destName;

                if (!sameParent)
                {
                    srcItem->parent = destDir;
                    destDir->children.push_back(srcItem);
                }

                cout << "Moved: " << source << " -> " << dest << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void get(const string &filename)
    {
        try
        {
            Item *file = getItem(filename);
            if (!file || file->isFolder)
                throw runtime_error("File not found: " + filename);

            cout << file->content << endl;

            string fileName;
            int lastSlash = filename.find_last_of('/');
            if (lastSlash != string::npos)
                fileName = filename.substr(lastSlash + 1);
            else
                fileName = filename;

            ofstream outFile(fileName);
            if (!outFile.is_open())
                throw runtime_error("Cannot create file: " + fileName);

            outFile << file->content;
            outFile.close();
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void put(const string &realFile, const string &fsPath)
    {
        try
        {
            ifstream file(realFile);
            if (!file.is_open())
                throw runtime_error("Cannot open real file: " + realFile);

            string content, line;
            while (getline(file, line))
                content += line + "\n";
            file.close();

            if (!content.empty() && content.back() == '\n')
                content.pop_back();

            Item *destDir = getItem(fsPath);
            if (!destDir || !destDir->isFolder)
                throw runtime_error("Destination directory not found");

            for (Item *child : destDir->children)
            {
                if (!child->isFolder && realFile == child->name)
                    throw runtime_error("File already exists: " + realFile);
            }

            Item *newFile = new Item();
            newFile->isFolder = false;
            newFile->name = realFile;
            newFile->content = content;
            newFile->parent = destDir;

            destDir->children.push_back(newFile);
            saveToDisk(newFile);

            cout << "File copied from real system: " << realFile
                 << " -> " << fsPath << endl;
            cout << newFile->content << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void info(const string &filename)
    {
        try
        {
            Item *file = getItem(filename);
            if (!file)
                throw runtime_error("File not found: " + filename);

            cout << "Name: " << file->name << endl;
            cout << "Path: " << getFullPath(file) << endl;
            if (!file->isFolder)
            {
                cout << "Size: " << file->content.length() << " bytes" << endl;
                if (!file->sectors.empty())
                {
                    cout << "Sectors: ";
                    for (int sector : file->sectors)
                        cout << sector << " ";
                    cout << endl;
                }
            }
            else
            {
                cout << "Type: Directory" << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    void defrag()
    {
        try
        {
            cout << "Starting disk defragmentation..." << endl;

            vector<Item *> allFiles;
            collectAllFiles(root, allFiles);

            cout << "Found " << allFiles.size() << " files" << endl;

            for (int i = 0; i < sectorMap.size(); i++)
                sectorMap[i] = false;

            int nextSector = 0;
            for (Item *file : allFiles)
            {
                if (file->isFolder)
                    continue;

                file->sectors.clear();

                string data = file->content;
                int pos = 0;
                while (pos < data.length())
                {
                    if (nextSector >= totalSectors)
                        throw runtime_error("Disk is full");

                    sectorMap[nextSector] = true;
                    file->sectors.push_back(nextSector);

                    while (nextSector >= disk.size())
                        disk.push_back("");

                    int len = min(SECTOR_SIZE, (int)data.length() - pos);
                    disk[nextSector] = data.substr(pos, len);
                    pos += len;
                    nextSector++;
                }
            }

            cout << "Defragmentation completed successfully!" << endl;
            cout << "Used sectors: 0 to " << (nextSector - 1) << endl;
            cout << "Free sectors: " << (totalSectors - nextSector) << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error during defragmentation: " << e.what() << endl;
        }
    }
};

void printHelp()
{
    cout << "\n=== Available Commands ===" << endl;
    cout << "pwd                     - Print working directory" << endl;
    cout << "cd <path>               - Change directory" << endl;
    cout << "ls [path]               - List directory contents" << endl;
    cout << "mkdir <name>            - Create directory" << endl;
    cout << "touch <name>            - Create file" << endl;
    cout << "rm <name>               - Remove file" << endl;
    cout << "rm -r <name>            - Remove directory recursively" << endl;
    cout << "cp <source> <dest>      - Copy file or directory" << endl;
    cout << "mv <source> <dest>      - Move/rename file or directory" << endl;
    cout << "get <file>              - Display file content" << endl;
    cout << "put <real> <virtual>    - Copy real file to virtual FS" << endl;
    cout << "info <file>             - Display file information" << endl;
    cout << "defrag                  - Defragment disk" << endl;
    cout << "help                    - Show this help" << endl;
    cout << "exit                    - Exit program" << endl;
    cout << "================================\n"
         << endl;
}

int main()
{
    cout << "=== File System ===" << endl;

    int diskCapacity;
    cout << "Enter disk capacity (number of sectors): ";
    while (cin >> diskCapacity)
    {
        if (diskCapacity <= 0)
        {
            cerr << "Error: Disk capacity must be positive" << endl;
            cout << "Enter disk capacity (number of sectors): ";
        }
        else
            break;
    }
    cin.ignore();

    FileSystem fs(diskCapacity);
    cout << "File system created with " << diskCapacity << " sectors" << endl;
    printHelp();

    string input;
    while (true)
    {
        cout << "fs:$ ";
        getline(cin, input);

        if (input.empty())
            continue;

        vector<string> tokens;
        istringstream stream(input);
        string token;

        while (stream >> token)
            tokens.push_back(token);

        if (tokens.empty())
            continue;

        string command = tokens[0];

        if (command == "exit" || command == "quit")
        {
            cout << "Goodbye!" << endl;
            break;
        }
        else if (command == "help")
            printHelp();
        else if (command == "pwd")
            fs.pwd();
        else if (command == "cd")
        {
            if (tokens.size() < 2)
                cerr << "Error: cd requires a path" << endl;
            else
                fs.cd(tokens[1]);
        }
        else if (command == "ls")
        {
            if (tokens.size() > 1)
                fs.ls(tokens[1]);
            else
                fs.ls();
        }
        else if (command == "mkdir")
        {
            if (tokens.size() < 2)
                cerr << "Error: mkdir requires a name" << endl;
            else
                fs.mkdir(tokens[1]);
        }
        else if (command == "touch")
        {
            if (tokens.size() < 2)
                cerr << "Error: touch requires a filename" << endl;
            else
                fs.touch(tokens[1]);
        }
        else if (command == "rm")
        {
            if (tokens.size() < 2)
                cerr << "Error: rm requires a name" << endl;
            else if (tokens.size() == 3 && tokens[1] == "-r")
                fs.rm(tokens[2], true);
            else
                fs.rm(tokens[1], false);
        }
        else if (command == "cp")
        {
            if (tokens.size() < 3)
                cerr << "Error: cp requires source and destination" << endl;
            else
                fs.cp(tokens[1], tokens[2]);
        }
        else if (command == "mv")
        {
            if (tokens.size() < 3)
                cerr << "Error: mv requires source and destination" << endl;
            else
                fs.mv(tokens[1], tokens[2]);
        }
        else if (command == "get")
        {
            if (tokens.size() < 2)
                cerr << "Error: get requires a filename" << endl;
            else
                fs.get(tokens[1]);
        }
        else if (command == "put")
        {
            if (tokens.size() < 3)
                cerr << "Error: put requires real file and virtual file names" << endl;
            else
                fs.put(tokens[1], tokens[2]);
        }
        else if (command == "info")
        {
            if (tokens.size() < 2)
                cerr << "Error: info requires a filename" << endl;
            else
                fs.info(tokens[1]);
        }
        else if (command == "defrag")
            fs.defrag();
        else
        {
            cerr << "Error: Unknown command: " << command << endl;
            cout << "Type 'help' for available commands" << endl;
        }
    }

    return 0;
}
