#include "TrayIcon.hpp"
#include <QApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QIcon>
#include <spdlog/spdlog.h>

namespace luma::ui {

TrayIcon::TrayIcon(MainWindow* mainWindow, QObject *parent)
    : QSystemTrayIcon(parent), m_mainWindow(mainWindow) {
    
    // Set a placeholder icon (we should ideally load an actual resource here)
    // Create a 32x32 blank pixmap for now since we have no icon asset
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::blue);
    setIcon(QIcon(pixmap));

    m_menu = new QMenu();
    
    QAction* settingsAction = m_menu->addAction("Settings...");
    m_menu->addSeparator();
    
    m_playPauseAction = m_menu->addAction("Pause");
    QAction* nextAction = m_menu->addAction("Next Wallpaper");
    QAction* prevAction = m_menu->addAction("Previous Wallpaper");
    m_menu->addSeparator();
    
    QAction* quitAction = m_menu->addAction("Quit LumaWall");

    connect(settingsAction, &QAction::triggered, this, &TrayIcon::toggleMainWindow);
    connect(m_playPauseAction, &QAction::triggered, this, &TrayIcon::onPlayPause);
    connect(nextAction, &QAction::triggered, this, &TrayIcon::onNext);
    connect(prevAction, &QAction::triggered, this, &TrayIcon::onPrevious);
    connect(quitAction, &QAction::triggered, this, &TrayIcon::onQuit);

    connect(this, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            toggleMainWindow();
        }
    });

    setContextMenu(m_menu);
}

TrayIcon::~TrayIcon() = default;

void TrayIcon::toggleMainWindow() {
    if (m_mainWindow->isVisible()) {
        m_mainWindow->hide();
    } else {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
}

void TrayIcon::sendDBusMessage(const QString& method) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.lumawall.Daemon",
        "/org/lumawall/Daemon",
        "org.lumawall.Daemon",
        method
    );

    if (!QDBusConnection::sessionBus().send(msg)) {
        spdlog::error("[UI] Failed to send D-Bus message: {}", method.toStdString());
    }
}

void TrayIcon::onPlayPause() {
    // Basic toggle logic (UI state might drift if daemon state changes independently, needs property binding later)
    if (m_playPauseAction->text() == "Pause") {
        sendDBusMessage("Pause");
        m_playPauseAction->setText("Play");
    } else {
        sendDBusMessage("Play");
        m_playPauseAction->setText("Pause");
    }
}

void TrayIcon::onNext() {
    sendDBusMessage("Next");
}

void TrayIcon::onPrevious() {
    sendDBusMessage("Previous");
}

void TrayIcon::onQuit() {
    sendDBusMessage("Quit");
    QApplication::quit();
}

} // namespace luma::ui
