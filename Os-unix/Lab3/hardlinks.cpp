#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// Хэш FNV-1a (64 бита), файл читается блоками
static bool fileHash(const fs::path &path, uint64_t &hash) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    hash = 1469598103934665603ULL;
    char buf[65536];
    while (in.read(buf, sizeof buf) || in.gcount() > 0) {
        for (std::streamsize i = 0; i < in.gcount(); i++) {
            hash ^= (unsigned char)buf[i];
            hash *= 1099511628211ULL;
        }
    }
    return !in.bad();
}

// Побайтовое сравнение: защита от совпадения хэшей у разных файлов
static bool sameContent(const fs::path &a, const fs::path &b) {
    std::error_code ec1, ec2;
    if (fs::file_size(a, ec1) != fs::file_size(b, ec2) || ec1 || ec2)
        return false;

    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    if (!fa || !fb)
        return false;

    static char ba[65536], bb[65536];
    while (fa && fb) {
        fa.read(ba, sizeof ba);
        fb.read(bb, sizeof bb);
        if (fa.gcount() != fb.gcount() || std::memcmp(ba, bb, (size_t)fa.gcount()) != 0)
            return false;
    }
    return true;
}

// Замена дубликата жёсткой ссылкой: ссылка под временным именем, затем rename.
// При ошибке дубликат остаётся на месте.
static bool replaceWithLink(const fs::path &original, const fs::path &duplicate) {
    fs::path tmp = duplicate;
    tmp += ".hardlink.tmp";

    std::error_code ec;
    fs::create_hard_link(original, tmp, ec);
    if (!ec)
        fs::rename(tmp, duplicate, ec);
    if (ec) {
        std::cerr << "Ошибка: " << duplicate.string() << ": " << ec.message() << "\n";
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2 || !fs::is_directory(argv[1])) {
        std::cerr << "Использование: " << argv[0] << " <каталог>\n";
        return 1;
    }

    // 1. Обход каталога и список файлов
    std::vector<fs::path> files;
    std::error_code ec;
    fs::recursive_directory_iterator it(argv[1], fs::directory_options::skip_permission_denied, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
        if (fs::is_regular_file(it->symlink_status()))
            files.push_back(it->path());

    // 2. Хэш каждого файла, 3. замена дубликатов жёсткими ссылками
    std::unordered_map<uint64_t, std::vector<fs::path>> kept;
    int linked = 0, errors = 0;

    for (const fs::path &file : files) {
        uint64_t hash;
        if (!fileHash(file, hash)) {
            std::cerr << "Не удалось прочитать: " << file.string() << "\n";
            errors++;
            continue;
        }

        bool duplicate = false;
        for (const fs::path &original : kept[hash]) {
            if (fs::equivalent(original, file, ec)) {
                duplicate = true;
                break;
            }
            if (sameContent(original, file)) {
                duplicate = true;
                if (replaceWithLink(original, file)) {
                    std::cout << file.string() << " -> " << original.string() << "\n";
                    linked++;
                } else {
                    errors++;
                }
                break;
            }
        }
        if (!duplicate)
            kept[hash].push_back(file);
    }

    std::cout << "Файлов: " << files.size() << ", заменено ссылками: " << linked
              << ", ошибок: " << errors << "\n";
    return errors ? 1 : 0;
}
