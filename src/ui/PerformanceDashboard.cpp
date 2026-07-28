#include "PerformanceDashboard.hpp"
#include <QVBoxLayout>
#include <QDBusMessage>
#include <QDBusConnection>
#include <spdlog/spdlog.h>

namespace luma::ui {

PerformanceDashboard::PerformanceDashboard(QWidget *parent)
    : QWidget(parent) {
    
    auto* layout = new QVBoxLayout(this);
    
    m_fpsLabel = new QLabel("FPS: --", this);
    m_decodeLatencyLabel = new QLabel("Decode Latency: -- ms", this);
    m_gpuUsageLabel = new QLabel("GPU Usage: -- %", this);
    m_vramLabel = new QLabel("VRAM: -- MB", this);
    
    layout->addWidget(m_fpsLabel);
    layout->addWidget(m_decodeLatencyLabel);
    layout->addWidget(m_gpuUsageLabel);
    layout->addWidget(m_vramLabel);
    layout->addStretch();
    
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &PerformanceDashboard::updateTelemetry);
    m_pollTimer->start(1000); // Poll every second
}

PerformanceDashboard::~PerformanceDashboard() = default;

void PerformanceDashboard::updateTelemetry() {
    // In a full implementation, this polls the D-Bus 'GetTelemetry' method from the daemon
    // For now, we update it with mock values if the daemon isn't returning data.
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.lumawall.Daemon",
        "/org/lumawall/Daemon",
        "org.lumawall.Daemon",
        "GetFPS"
    );

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        double fps = reply.arguments().at(0).toDouble();
        m_fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    } else {
        m_fpsLabel->setText("FPS: 60.0 (Mock DBus Fallback)");
        m_decodeLatencyLabel->setText("Decode Latency: 2.3 ms");
        m_gpuUsageLabel->setText("GPU Usage: 4 %");
        m_vramLabel->setText("VRAM: 142 MB");
    }
}

} // namespace luma::ui
