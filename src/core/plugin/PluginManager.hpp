#pragma once

#include <QString>
#include <QVector>
#include <memory>
#include "IPlugin.hpp"

namespace luma::plugin {

class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    bool loadPlugin(const QString& path);
    void loadAllPlugins(const QString& directoryPath);
    void shutdownAll();

    const QVector<IPlugin*>& getLoadedPlugins() const;

private:
    QVector<IPlugin*> m_plugins;
};

} // namespace luma::plugin
