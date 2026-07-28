#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>

namespace luma::ui {

class PerformanceDashboard : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceDashboard(QWidget *parent = nullptr);
    ~PerformanceDashboard();

private slots:
    void updateTelemetry();

private:
    QLabel* m_fpsLabel;
    QLabel* m_decodeLatencyLabel;
    QLabel* m_gpuUsageLabel;
    QLabel* m_vramLabel;
    
    QTimer* m_pollTimer;
};

} // namespace luma::ui
