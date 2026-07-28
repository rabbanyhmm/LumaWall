#pragma once

#include <QString>
#include <QtPlugin>

namespace luma::plugin {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual QString getName() const = 0;
    virtual QString getVersion() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

} // namespace luma::plugin

#define LumaWall_IPlugin_iid "org.lumawall.IPlugin"
Q_DECLARE_INTERFACE(luma::plugin::IPlugin, LumaWall_IPlugin_iid)
