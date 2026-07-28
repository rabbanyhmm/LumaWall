#include "OutputManager.hpp"
#include <core/Logging.hpp>
#include <cstdlib>

namespace luma::platform::x11::randr {

OutputManager::OutputManager(xcb_connection_t* conn, xcb_screen_t* screen, std::shared_ptr<luma::core::events::EventBus> eventBus)
    : m_conn(conn), m_screen(screen), m_eventBus(std::move(eventBus)) {}

bool OutputManager::init() {
    const xcb_query_extension_reply_t* ext = xcb_get_extension_data(m_conn, &xcb_randr_id);
    if (!ext || !ext->present) {
        spdlog::error("[X11] RandR extension not found");
        return false;
    }
    m_randrEventBase = ext->first_event;

    xcb_randr_query_version_cookie_t vCookie = xcb_randr_query_version(m_conn, 1, 5);
    xcb_randr_query_version_reply_t* vReply = xcb_randr_query_version_reply(m_conn, vCookie, nullptr);
    if (vReply) {
        spdlog::info("[X11] RandR version {}.{}", vReply->major_version, vReply->minor_version);
        free(vReply);
    }

    // Subscribe to RandR events
    xcb_randr_select_input(m_conn, m_screen->root, 
        XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE);
    
    queryOutputs();
    return true;
}

void OutputManager::queryOutputs() {
    xcb_randr_get_screen_resources_current_cookie_t resCookie = xcb_randr_get_screen_resources_current(m_conn, m_screen->root);
    xcb_randr_get_screen_resources_current_reply_t* resReply = xcb_randr_get_screen_resources_current_reply(m_conn, resCookie, nullptr);
    
    if (!resReply) return;

    int num_outputs = xcb_randr_get_screen_resources_current_outputs_length(resReply);
    xcb_randr_output_t* outputs = xcb_randr_get_screen_resources_current_outputs(resReply);

    for (int i = 0; i < num_outputs; ++i) {
        xcb_randr_get_output_info_cookie_t infoCookie = xcb_randr_get_output_info(m_conn, outputs[i], resReply->config_timestamp);
        xcb_randr_get_output_info_reply_t* info = xcb_randr_get_output_info_reply(m_conn, infoCookie, nullptr);
        
        if (info) {
            if (info->connection == XCB_RANDR_CONNECTION_CONNECTED && info->crtc != XCB_NONE) {
                xcb_randr_get_crtc_info_cookie_t crtcCookie = xcb_randr_get_crtc_info(m_conn, info->crtc, resReply->config_timestamp);
                xcb_randr_get_crtc_info_reply_t* crtc = xcb_randr_get_crtc_info_reply(m_conn, crtcCookie, nullptr);
                
                if (crtc) {
                    luma::core::events::MonitorAddedEvent ev;
                    ev.monitorId = std::to_string(outputs[i]);
                    ev.name = "X11 Display"; // Could fetch EDID property, omitting for brevity
                    ev.width = crtc->width;
                    ev.height = crtc->height;
                    
                    spdlog::info("[X11] Monitor Added: ID {}, {}x{}", outputs[i], crtc->width, crtc->height);
                    
                    if (m_eventBus) {
                        m_eventBus->pushEvent(std::move(ev));
                    }
                    free(crtc);
                }
            }
            free(info);
        }
    }
    
    free(resReply);
}

void OutputManager::handleRandrEvent(xcb_generic_event_t* event) {
    uint8_t type = event->response_type & 0x7F;
    if (type == m_randrEventBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
        spdlog::info("[X11] RandR Screen Change Notify");
        queryOutputs();
    }
}

} // namespace luma::platform::x11::randr
