#include "GnomeIntegration.hpp"
#include <app/DBusServer.hpp>
#include <spdlog/spdlog.h>
#include <QDebug>

namespace luma::platform::gnome {

GnomeIntegration::GnomeIntegration(std::shared_ptr<luma::app::DBusServer> dbusServer, std::function<void(bool)> pauseCallback)
    : m_dbusServer(dbusServer), m_pauseCallback(pauseCallback) {
}

GnomeIntegration::~GnomeIntegration() {
}

bool GnomeIntegration::initialize() {
    if (!m_dbusServer) return false;

    // Connect to the DBus server's desktop state signal
    connect(m_dbusServer.get(), &luma::app::DBusServer::desktopStateChanged,
            this, &GnomeIntegration::handleDesktopState);

    spdlog::info("[GnomeIntegration] Initialized and connected to DBus IPC.");
    return true;
}

common::DesktopState GnomeIntegration::getState() {
    return m_currentState;
}

void GnomeIntegration::onOverviewChanged(bool active) {
    if (active) {
        spdlog::info("[GnomeIntegration] Overview opened. Keeping video rendering active for BackgroundActor clone.");
    } else {
        spdlog::info("[GnomeIntegration] Overview closed. Video rendering continues.");
    }
}

void GnomeIntegration::handleDesktopState(const QString& state) {
    if (state == "overview") {
        m_currentState = common::DesktopState::Overview;
        onOverviewChanged(true);
    } else if (state == "normal") {
        m_currentState = common::DesktopState::Normal;
        onOverviewChanged(false);
        // Resume if we were locked
        spdlog::info("[GnomeIntegration] Desktop normal. Resuming rendering.");
        if (m_pauseCallback) m_pauseCallback(false);
    } else if (state == "locked") {
        m_currentState = common::DesktopState::Locked;
        spdlog::info("[GnomeIntegration] Screen locked. Pausing rendering.");
        if (m_pauseCallback) m_pauseCallback(true); // Pause on lock
    }
}

} // namespace luma::platform::gnome

#include "moc_GnomeIntegration.cpp"
