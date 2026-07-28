#include <QApplication>
#include "MainWindow.hpp"
#include "TrayIcon.hpp"
#include <library/DatabaseManager.hpp>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <spdlog/spdlog.h>
#include <memory>

void ensureDaemonRunning() {
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.lumawall.Daemon")) {
        spdlog::warn("[WATCHDOG] Daemon not running! Starting it...");
        QProcess::startDetached("lumawall", QStringList() << "--daemon");
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LumaWall");
    app.setQuitOnLastWindowClosed(false); // Keep running in tray

    if (!QDBusConnection::sessionBus().isConnected()) {
        spdlog::error("[UI] Cannot connect to the D-Bus session bus.");
        return 1;
    }

    auto db = std::make_shared<luma::library::DatabaseManager>();
    db->init(std::string(std::getenv("HOME")) + "/.local/share/lumawall/library.db");

    // Watchdog
    ensureDaemonRunning();
    QDBusServiceWatcher watchdog("org.lumawall.Daemon", QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration);
    QObject::connect(&watchdog, &QDBusServiceWatcher::serviceUnregistered, [](const QString&) {
        spdlog::error("[WATCHDOG] Daemon crashed or exited. Restarting...");
        ensureDaemonRunning();
    });

    luma::ui::MainWindow mainWindow(db);
    luma::ui::TrayIcon trayIcon(&mainWindow);

    trayIcon.show();

    return app.exec();
}
