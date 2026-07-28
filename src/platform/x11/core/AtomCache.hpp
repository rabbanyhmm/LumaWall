#pragma once
#include <xcb/xcb.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace luma::platform::x11::core {

class AtomCache {
public:
    AtomCache(xcb_connection_t* conn);
    ~AtomCache() = default;

    bool prefetch(const std::vector<std::string>& atomNames);
    xcb_atom_t get(const std::string& name) const;

private:
    xcb_connection_t* m_conn;
    std::unordered_map<std::string, xcb_atom_t> m_atoms;
};

} // namespace luma::platform::x11::core
