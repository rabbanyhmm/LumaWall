#include "MainWindow.hpp"
#include "WallpaperBrowser.hpp"
#include "PlaylistEditor.hpp"
#include "PerformanceDashboard.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

namespace luma::ui {

MainWindow::MainWindow(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent)
    : QMainWindow(parent), m_db(std::move(db)) {
    setWindowTitle("LumaWall Settings");
    resize(1000, 700);

    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Sidebar
    m_sidebar = new QListWidget(this);
    m_sidebar->setFixedWidth(200);
    m_sidebar->addItem("Home");
    m_sidebar->addItem("Wallpapers");
    m_sidebar->addItem("Playlists");
    m_sidebar->addItem("Performance");
    m_sidebar->addItem("Settings");
    m_sidebar->addItem("About");

    // Content Area
    m_contentArea = new QStackedWidget(this);

    // Page 0: Home (Placeholder)
    auto* homeLabel = new QLabel("LumaWall Home", this);
    homeLabel->setAlignment(Qt::AlignCenter);
    m_contentArea->addWidget(homeLabel);

    // Page 1: Wallpapers
    auto* wallpaperBrowser = new WallpaperBrowser(m_db, this);
    m_contentArea->addWidget(wallpaperBrowser);

    // Page 2: Playlists
    auto* playlistEditor = new PlaylistEditor(m_db, this);
    m_contentArea->addWidget(playlistEditor);

    // Page 3: Performance
    auto* perfDashboard = new PerformanceDashboard(this);
    m_contentArea->addWidget(perfDashboard);

    // Page 4: Settings (Placeholder)
    auto* settingsLabel = new QLabel("Settings (Coming Soon)", this);
    settingsLabel->setAlignment(Qt::AlignCenter);
    m_contentArea->addWidget(settingsLabel);

    // Page 5: About (Placeholder)
    auto* aboutLabel = new QLabel("LumaWall v0.1.0", this);
    aboutLabel->setAlignment(Qt::AlignCenter);
    m_contentArea->addWidget(aboutLabel);

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_contentArea);
    setCentralWidget(centralWidget);

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::switchPage);
    m_sidebar->setCurrentRow(1); // Default to Wallpapers
}

void MainWindow::switchPage(int index) {
    m_contentArea->setCurrentIndex(index);
}

} // namespace luma::ui
