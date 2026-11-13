#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <каталог>\n";
        return 1;
    }

    std::vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(argv[1]))
        if (fs::is_regular_file(entry.symlink_status()))
            files.push_back(entry.path());

    std::map<size_t, fs::path> kept;

    for (const auto &file : files) {
        std::ifstream in(file, std::ios::binary);
        if (!in)
            continue;
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        size_t hash = std::hash<std::string>{}(data);

        auto [found, isNew] = kept.emplace(hash, file);
        if (!isNew) {
            fs::remove(file);
            fs::create_hard_link(found->second, file);
        }
    }
}
