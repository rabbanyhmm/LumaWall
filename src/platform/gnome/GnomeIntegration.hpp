#pragma once
#include <platform/common/IDesktopIntegration.hpp>
#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace luma::app {
    class DBusServer;
}

namespace luma::platform::gnome {

class GnomeIntegration : public QObject, public luma::platform::common::IDesktopIntegration {
    Q_OBJECT
public:
    GnomeIntegration(std::shared_ptr<luma::app::DBusServer> dbusServer, std::function<void(bool)> pauseCallback);
    ~GnomeIntegration() override;

    bool initialize() override;
    common::DesktopState getState() override;
    void onOverviewChanged(bool active) override;

public slots:
    void handleDesktopState(const QString& state);

private:
    std::shared_ptr<luma::app::DBusServer> m_dbusServer;
    common::DesktopState m_currentState = common::DesktopState::Normal;
    std::function<void(bool)> m_pauseCallback;
};

} // namespace luma::platform::gnome
