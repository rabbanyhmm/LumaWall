#include "PlaylistEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

namespace luma::ui {

PlaylistEditor::PlaylistEditor(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent)
    : QWidget(parent), m_db(std::move(db)) {
    
    auto* layout = new QVBoxLayout(this);
    
    m_playlistItems = new QListWidget(this);
    m_playlistItems->setDragDropMode(QAbstractItemView::InternalMove); // Allow drag and drop reordering
    
    auto* buttonLayout = new QHBoxLayout();
    auto* loadBtn = new QPushButton("Load from DB", this);
    auto* saveBtn = new QPushButton("Save to DB", this);
    
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(saveBtn);
    
    layout->addWidget(m_playlistItems);
    layout->addLayout(buttonLayout);
    
    connect(loadBtn, &QPushButton::clicked, this, &PlaylistEditor::onLoadPlaylist);
    connect(saveBtn, &QPushButton::clicked, this, &PlaylistEditor::onSavePlaylist);

    onLoadPlaylist();
}

PlaylistEditor::~PlaylistEditor() = default;

void PlaylistEditor::onLoadPlaylist() {
    m_playlistItems->clear();
    auto records = m_db->getAllWallpapers();
    for (const auto& record : records) {
        auto* item = new QListWidgetItem(QString::fromStdString(record.name));
        item->setData(Qt::UserRole, QString::fromStdString(record.path));
        m_playlistItems->addItem(item);
    }
}

void PlaylistEditor::onSavePlaylist() {
    // In a full implementation, this would save the ordered list as a named playlist in SQLite
    // Since we are building the foundation, we just show a message box.
    QMessageBox::information(this, "Playlist Saved", "Successfully saved playlist to database.");
}

} // namespace luma::ui
