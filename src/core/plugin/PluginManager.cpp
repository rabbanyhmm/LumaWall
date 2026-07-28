#include "PluginManager.hpp"
#include <core/Logging.hpp>
#include <QPluginLoader>
#include <QDir>

namespace luma::plugin {

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    shutdownAll();
}

bool PluginManager::loadPlugin(const QString& path) {
    QPluginLoader loader(path);
    QObject* pluginObj = loader.instance();

    if (pluginObj) {
        IPlugin* plugin = qobject_cast<IPlugin*>(pluginObj);
        if (plugin) {
            if (plugin->initialize()) {
                m_plugins.append(plugin);
                spdlog::info("[PLUGIN] Successfully loaded plugin: {} v{}", 
                             plugin->getName().toStdString(), 
                             plugin->getVersion().toStdString());
                return true;
            } else {
                spdlog::error("[PLUGIN] Failed to initialize plugin: {}", path.toStdString());
                loader.unload();
            }
        } else {
            spdlog::error("[PLUGIN] Plugin at {} does not implement IPlugin interface", path.toStdString());
            loader.unload();
        }
    } else {
        spdlog::error("[PLUGIN] Failed to load plugin library {}: {}", path.toStdString(), loader.errorString().toStdString());
    }

    return false;
}

void PluginManager::loadAllPlugins(const QString& directoryPath) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        spdlog::warn("[PLUGIN] Plugin directory does not exist: {}", directoryPath.toStdString());
        return;
    }

    // On Linux, shared libraries end with .so
    QStringList filters;
    filters << "*.so";
    dir.setNameFilters(filters);

    for (const QFileInfo& fileInfo : dir.entryInfoList(QDir::Files)) {
        loadPlugin(fileInfo.absoluteFilePath());
    }
}

void PluginManager::shutdownAll() {
    for (IPlugin* plugin : m_plugins) {
        spdlog::info("[PLUGIN] Shutting down plugin: {}", plugin->getName().toStdString());
        plugin->shutdown();
    }
    m_plugins.clear();
}

const QVector<IPlugin*>& PluginManager::getLoadedPlugins() const {
    return m_plugins;
}

} // namespace luma::plugin
