#pragma once

#include <QObject>
#include <QtDBus/QtDBus>
#include <library/PlaylistEngine.hpp>
#include <memory>
#include <atomic>

namespace luma::app {

class DBusServer : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.lumawall.Daemon")

public:
    explicit DBusServer(std::shared_ptr<library::PlaylistEngine> playlistEngine, QObject* parent = nullptr);
    ~DBusServer();

    bool registerService();

public slots:
    // D-Bus exported methods
    void Play();
    void PlayFile(const QString& path);
    void PlayFileForMonitor(const QString& monitorId, const QString& path);
    void Pause();
    void PauseMonitor(const QString& monitorId);
    void Next();
    void NextMonitor(const QString& monitorId);
    void Previous();
    void PreviousMonitor(const QString& monitorId);
    void Quit();
    void SetDesktopState(const QString& state);

    bool IsPlaying() const;
    bool IsMonitorPlaying(const QString& monitorId) const;

signals:
    // Local signals to the main loop/engine
    void requestPlay();
    void requestPlayFile(const QString& monitorId, const QString& path);
    void requestPause(const QString& monitorId);
    void requestNext(const QString& monitorId);
    void requestPrevious(const QString& monitorId);
    void requestQuit();

    // Exported signals
    void errorOccurred(int code, const QString& message, const QString& subsystem);
    void desktopStateChanged(const QString& state);

private:
    std::shared_ptr<library::PlaylistEngine> m_playlistEngine;
    std::atomic<bool> m_isPlaying{true};
};

} // namespace luma::app
