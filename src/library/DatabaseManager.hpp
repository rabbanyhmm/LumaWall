#pragma once

#include <string>
#include <vector>
#include <memory>
#include <SQLiteCpp/SQLiteCpp.h>

namespace luma::library {

struct WallpaperRecord {
    int id{0};
    std::string path;
    std::string name;
    double duration{0.0};
    int width{0};
    int height{0};
    std::string format;
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool init(const std::string& dbPath);

    bool addWallpaper(const WallpaperRecord& record);
    std::vector<WallpaperRecord> getAllWallpapers();
    bool removeWallpaper(int id);

private:
    void migrate();
    void createTables();

    std::unique_ptr<SQLite::Database> m_db;
};

} // namespace luma::library
