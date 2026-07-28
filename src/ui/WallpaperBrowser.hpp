#pragma once

#include <QWidget>
#include <QListWidget>
#include <library/DatabaseManager.hpp>
#include <library/ThumbnailCache.hpp>
#include <memory>

namespace luma::ui {

class WallpaperBrowser : public QWidget {
    Q_OBJECT

public:
    explicit WallpaperBrowser(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent = nullptr);
    ~WallpaperBrowser();

private slots:
    void onWallpaperDoubleClicked(QListWidgetItem* item);

private:
    void loadWallpapers();

    QListWidget* m_listWidget;
    std::shared_ptr<luma::library::DatabaseManager> m_db;
    std::shared_ptr<luma::library::ThumbnailCache> m_thumbnailCache;
};

} // namespace luma::ui
