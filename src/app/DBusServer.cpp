#include "DBusServer.hpp"
#include <core/Logging.hpp>
#include <core/SettingsManager.hpp>
#include <core/error/ErrorTypes.hpp>
#include <QDBusConnection>
#include <QDBusError>

namespace luma::app {

DBusServer::DBusServer(std::shared_ptr<library::PlaylistEngine> playlistEngine, QObject* parent)
    : QObject(parent), m_playlistEngine(std::move(playlistEngine)) {
    
    // Wire up core error reporting to D-Bus signals
    core::error::ErrorHandler::instance().registerCallback([this](const core::error::ErrorDetails& details) {
        emit errorOccurred(static_cast<int>(details.code), QString::fromStdString(details.message), QString::fromStdString(details.subsystem));
    });
}

DBusServer::~DBusServer() {
    QDBusConnection::sessionBus().unregisterService("org.lumawall.Daemon");
}

bool DBusServer::registerService() {
    QDBusConnection connection = QDBusConnection::sessionBus();
    
    if (!connection.registerService("org.lumawall.Daemon")) {
        spdlog::error("[DBUS] Failed to register service: {}", qPrintable(connection.lastError().message()));
        return false;
    }

    if (!connection.registerObject("/org/lumawall/Daemon", this, QDBusConnection::ExportAllSlots)) {
        spdlog::error("[DBUS] Failed to register object: {}", qPrintable(connection.lastError().message()));
        return false;
    }

    spdlog::info("[DBUS] Successfully registered org.lumawall.Daemon on session bus");
    return true;
}

void DBusServer::Play() {
    spdlog::info("[DBUS] Received Play command");
    m_isPlaying = true;
    emit requestPlay();
}

void DBusServer::PlayFile(const QString& path) {
    PlayFileForMonitor("DP-1", path); // Fallback for old clients
}

void DBusServer::PlayFileForMonitor(const QString& monitorId, const QString& path) {
    spdlog::info("[DBUS] Received PlayFile command for monitor {}: {}", monitorId.toStdString(), path.toStdString());
    core::SettingsManager::instance().setActiveWallpaper(monitorId.toStdString(), path.toStdString());
    m_isPlaying = true;
    emit requestPlayFile(monitorId, path);
}

void DBusServer::Pause() {
    PauseMonitor("");
}

void DBusServer::PauseMonitor(const QString& monitorId) {
    spdlog::info("[DBUS] Received Pause command for monitor {}", monitorId.toStdString());
    m_isPlaying = false;
    emit requestPause(monitorId);
}

void DBusServer::Next() {
    NextMonitor("");
}

void DBusServer::NextMonitor(const QString& monitorId) {
    spdlog::info("[DBUS] Received Next command for monitor {}", monitorId.toStdString());
    emit requestNext(monitorId);
}

void DBusServer::Previous() {
    PreviousMonitor("");
}

void DBusServer::PreviousMonitor(const QString& monitorId) {
    spdlog::info("[DBUS] Received Previous command for monitor {}", monitorId.toStdString());
    emit requestPrevious(monitorId);
}

void DBusServer::Quit() {
    spdlog::info("[DBUS] Received Quit command");
    emit requestQuit();
}

void DBusServer::SetDesktopState(const QString& state) {
    spdlog::info("[DBUS] Received DesktopState change: {}", state.toStdString());
    emit desktopStateChanged(state);
}

bool DBusServer::IsPlaying() const {
    return m_isPlaying.load();
}

bool DBusServer::IsMonitorPlaying(const QString& monitorId) const {
    // Basic implementation for now
    return m_isPlaying.load();
}

} // namespace luma::app
