#pragma once

#include <QMainWindow>
#include <QLabel>

#include <QListWidget>
#include <QStackedWidget>
#include <library/DatabaseManager.hpp>
#include <memory>

namespace luma::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<luma::library::DatabaseManager> db, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchPage(int index);

private:
    void setupUi();

    std::shared_ptr<luma::library::DatabaseManager> m_db;
    QListWidget* m_sidebar;
    QStackedWidget* m_contentArea;
};

} // namespace luma::ui
