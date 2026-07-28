#pragma once

#include <QString>
#include <QSettings>

namespace luma::core {

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    // Returns a singleton instance
    static SettingsManager& instance();

    void initialize();
    
    // Global preferences
    int getVolume() const;
    void setVolume(int volume);

    bool useHardwareDecode() const;
    void setHardwareDecode(bool enabled);

    // Per-monitor preferences
    std::string getActiveWallpaper(const std::string& monitorId) const;
    void setActiveWallpaper(const std::string& monitorId, const std::string& path);
    
    std::string getScalingMode(const std::string& monitorId) const;
    void setScalingMode(const std::string& monitorId, const std::string& scaling);

    int getConfigVersion() const;

private:
    void migrateConfig();

    QSettings* m_settings;
    const int CURRENT_CONFIG_VERSION = 1;
};

} // namespace luma::core
