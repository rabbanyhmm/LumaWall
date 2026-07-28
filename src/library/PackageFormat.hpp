#pragma once

#include <string>
#include <vector>
#include "DatabaseManager.hpp"
#include <memory>

namespace luma::library {

class PackageFormat {
public:
    explicit PackageFormat(std::shared_ptr<DatabaseManager> db);
    ~PackageFormat();

    // Imports a .wallpkg file into the local storage and registers it in the database.
    // Returns the new database ID if successful, or -1 if failed.
    int importPackage(const std::string& packagePath);

private:
    bool extractZip(const std::string& zipPath, const std::string& destDir);
    bool parseManifestAndRegister(const std::string& destDir, int& outId);

    std::shared_ptr<DatabaseManager> m_db;
    std::string m_storageDir;
};

} // namespace luma::library
