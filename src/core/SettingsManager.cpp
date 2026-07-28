#include "SettingsManager.hpp"
#include <QDir>
#include <QStandardPaths>
#include <core/Logging.hpp>

namespace luma::core {

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/lumawall/config.ini";
    QDir().mkpath(QFileInfo(configPath).absolutePath());
    m_settings = new QSettings(configPath, QSettings::IniFormat);
}

SettingsManager::~SettingsManager() {
    delete m_settings;
}

void SettingsManager::initialize() {
    migrateConfig();
}

void SettingsManager::migrateConfig() {
    int version = getConfigVersion();
    
    if (version < CURRENT_CONFIG_VERSION) {
        spdlog::info("[SETTINGS] Migrating config from version {} to {}", version, CURRENT_CONFIG_VERSION);
        
        // Example migration logic here
        // if (version == 0) { ... }
        
        m_settings->setValue("config_version", CURRENT_CONFIG_VERSION);
        m_settings->sync();
    }
}

int SettingsManager::getConfigVersion() const {
    return m_settings->value("config_version", 0).toInt();
}

int SettingsManager::getVolume() const {
    return m_settings->value("Playback/volume", 100).toInt();
}

void SettingsManager::setVolume(int volume) {
    m_settings->setValue("Playback/volume", volume);
}

bool SettingsManager::useHardwareDecode() const {
    return m_settings->value("Renderer/hardware_decode", true).toBool();
}

void SettingsManager::setHardwareDecode(bool enabled) {
    m_settings->setValue("Renderer/hardware_decode", enabled);
}

std::string SettingsManager::getActiveWallpaper(const std::string& monitorId) const {
    return m_settings->value(QString("Monitors/%1/active_wallpaper").arg(monitorId.c_str()), "").toString().toStdString();
}

void SettingsManager::setActiveWallpaper(const std::string& monitorId, const std::string& path) {
    m_settings->setValue(QString("Monitors/%1/active_wallpaper").arg(monitorId.c_str()), QString::fromStdString(path));
}

std::string SettingsManager::getScalingMode(const std::string& monitorId) const {
    return m_settings->value(QString("Monitors/%1/scaling_mode").arg(monitorId.c_str()), "Fill").toString().toStdString();
}

void SettingsManager::setScalingMode(const std::string& monitorId, const std::string& scaling) {
    m_settings->setValue(QString("Monitors/%1/scaling_mode").arg(monitorId.c_str()), QString::fromStdString(scaling));
}

} // namespace luma::core
