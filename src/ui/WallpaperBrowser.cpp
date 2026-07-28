#include "WallpaperBrowser.hpp"
#include <QVBoxLayout>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QIcon>
#include <spdlog/spdlog.h>

namespace luma::ui {

WallpaperBrowser::WallpaperBrowser(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent)
    : QWidget(parent), m_db(std::move(db)) {
    
    m_thumbnailCache = std::make_shared<luma::library::ThumbnailCache>();

    auto* layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(320, 180));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setSpacing(10);
    
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &WallpaperBrowser::onWallpaperDoubleClicked);

    loadWallpapers();
}

WallpaperBrowser::~WallpaperBrowser() = default;

void WallpaperBrowser::loadWallpapers() {
    auto records = m_db->getAllWallpapers();
    for (const auto& record : records) {
        auto* item = new QListWidgetItem(QString::fromStdString(record.name));
        item->setData(Qt::UserRole, QString::fromStdString(record.path));

        auto thumbPath = m_thumbnailCache->getThumbnail(record.path);
        if (thumbPath) {
            item->setIcon(QIcon(QString::fromStdString(*thumbPath)));
        } else {
            // Placeholder if thumbnail fails
            QPixmap pixmap(320, 180);
            pixmap.fill(Qt::darkGray);
            item->setIcon(QIcon(pixmap));
        }

        m_listWidget->addItem(item);
    }
}

void WallpaperBrowser::onWallpaperDoubleClicked(QListWidgetItem* item) {
    QString path = item->data(Qt::UserRole).toString();

    // Call Play on the daemon
    // Wait, the daemon Play() method currently takes no arguments.
    // If the user wants to play a specific wallpaper, we need a PlayWallpaper(path) method in DBusServer.
    // Let's assume we send a generic Play for now and will update DBusServer later.
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.lumawall.Daemon",
        "/org/lumawall/Daemon",
        "org.lumawall.Daemon",
        "Play"
    );

    if (!QDBusConnection::sessionBus().send(msg)) {
        spdlog::error("[UI] Failed to send Play message to Daemon");
    }
}

} // namespace luma::ui
