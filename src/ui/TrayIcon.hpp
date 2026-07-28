#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include "MainWindow.hpp"

namespace luma::ui {

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit TrayIcon(MainWindow* mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

private slots:
    void onPlayPause();
    void onNext();
    void onPrevious();
    void onQuit();
    void toggleMainWindow();

private:
    void sendDBusMessage(const QString& method);

    MainWindow* m_mainWindow;
    QMenu* m_menu;
    QAction* m_playPauseAction;
};

} // namespace luma::ui
