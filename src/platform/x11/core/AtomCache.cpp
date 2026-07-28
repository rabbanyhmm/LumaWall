#include "AtomCache.hpp"
#include <core/Logging.hpp>
#include <cstdlib>

namespace luma::platform::x11::core {

AtomCache::AtomCache(xcb_connection_t* conn) : m_conn(conn) {}

bool AtomCache::prefetch(const std::vector<std::string>& atomNames) {
    std::vector<std::pair<std::string, xcb_intern_atom_cookie_t>> cookies;
    cookies.reserve(atomNames.size());

    for (const auto& name : atomNames) {
        cookies.push_back({
            name, 
            xcb_intern_atom(m_conn, 0, static_cast<uint16_t>(name.length()), name.c_str())
        });
    }

    bool success = true;
    for (auto& pair : cookies) {
        xcb_generic_error_t* err = nullptr;
        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(m_conn, pair.second, &err);
        
        if (err) {
            spdlog::error("[X11] Failed to fetch atom: {}", pair.first);
            free(err);
            success = false;
        } else if (reply) {
            m_atoms[pair.first] = reply->atom;
            free(reply);
        }
    }
    
    return success;
}

xcb_atom_t AtomCache::get(const std::string& name) const {
    auto it = m_atoms.find(name);
    if (it != m_atoms.end()) {
        return it->second;
    }
    return XCB_NONE;
}

} // namespace luma::platform::x11::core
