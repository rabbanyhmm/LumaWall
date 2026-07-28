#pragma once

#include <QWidget>
#include <QListWidget>
#include <library/DatabaseManager.hpp>
#include <memory>

namespace luma::ui {

class PlaylistEditor : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistEditor(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent = nullptr);
    ~PlaylistEditor();

private slots:
    void onSavePlaylist();
    void onLoadPlaylist();

private:
    std::shared_ptr<luma::library::DatabaseManager> m_db;
    QListWidget* m_playlistItems;
};

} // namespace luma::ui
