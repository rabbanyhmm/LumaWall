#include "DatabaseManager.hpp"
#include <core/Logging.hpp>
#include <filesystem>

namespace luma::library {

DatabaseManager::DatabaseManager() = default;
DatabaseManager::~DatabaseManager() = default;

bool DatabaseManager::init(const std::string& dbPath) {
    try {
        // Create directory if it doesn't exist
        std::filesystem::path path(dbPath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        m_db = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        spdlog::info("[LIBRARY] Database opened at {}", dbPath);

        migrate();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Database initialization failed: {}", e.what());
        return false;
    }
}

void DatabaseManager::createTables() {
    if (!m_db) return;

    try {
        const char* createWallpapersTable = R"(
            CREATE TABLE IF NOT EXISTS wallpapers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                path TEXT UNIQUE NOT NULL,
                name TEXT NOT NULL,
                duration REAL DEFAULT 0.0,
                width INTEGER DEFAULT 0,
                height INTEGER DEFAULT 0,
                format TEXT
            );
        )";
        
        m_db->exec(createWallpapersTable);
        spdlog::info("[LIBRARY] Database tables created/verified");
    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Failed to create tables: {}", e.what());
    }
}

void DatabaseManager::migrate() {
    if (!m_db) return;

    try {
        int currentVersion = m_db->execAndGet("PRAGMA user_version").getInt();
        int targetVersion = 2; // Target schema version

        spdlog::info("[LIBRARY] Database current version: {}, target: {}", currentVersion, targetVersion);

        if (currentVersion == 0) {
            // Version 0 to 1: Initial creation
            createTables();
            m_db->exec("PRAGMA user_version = 1");
            currentVersion = 1;
            spdlog::info("[LIBRARY] Migrated database to version 1");
        }

        if (currentVersion == 1) {
            // Version 1 to 2: For example, add a 'tags' column
            // We just do it to demonstrate the migration path
            m_db->exec("ALTER TABLE wallpapers ADD COLUMN tags TEXT DEFAULT ''");
            m_db->exec("PRAGMA user_version = 2");
            currentVersion = 2;
            spdlog::info("[LIBRARY] Migrated database to version 2");
        }

    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Migration failed: {}", e.what());
    }
}

bool DatabaseManager::addWallpaper(const WallpaperRecord& record) {
    if (!m_db) return false;

    try {
        SQLite::Statement query(*m_db, "INSERT OR IGNORE INTO wallpapers (path, name, duration, width, height, format) VALUES (?, ?, ?, ?, ?, ?)");
        query.bind(1, record.path);
        query.bind(2, record.name);
        query.bind(3, record.duration);
        query.bind(4, record.width);
        query.bind(5, record.height);
        query.bind(6, record.format);
        
        query.exec();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Failed to add wallpaper: {}", e.what());
        return false;
    }
}

std::vector<WallpaperRecord> DatabaseManager::getAllWallpapers() {
    std::vector<WallpaperRecord> wallpapers;
    if (!m_db) return wallpapers;

    try {
        SQLite::Statement query(*m_db, "SELECT id, path, name, duration, width, height, format FROM wallpapers");
        while (query.executeStep()) {
            WallpaperRecord record;
            record.id = query.getColumn(0).getInt();
            record.path = query.getColumn(1).getString();
            record.name = query.getColumn(2).getString();
            record.duration = query.getColumn(3).getDouble();
            record.width = query.getColumn(4).getInt();
            record.height = query.getColumn(5).getInt();
            record.format = query.getColumn(6).getString();
            wallpapers.push_back(record);
        }
    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Failed to retrieve wallpapers: {}", e.what());
    }

    return wallpapers;
}

bool DatabaseManager::removeWallpaper(int id) {
    if (!m_db) return false;

    try {
        SQLite::Statement query(*m_db, "DELETE FROM wallpapers WHERE id = ?");
        query.bind(1, id);
        query.exec();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[LIBRARY] Failed to remove wallpaper: {}", e.what());
        return false;
    }
}

} // namespace luma::library
