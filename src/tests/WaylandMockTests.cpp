#include <gtest/gtest.h>
#include <wayland-server.h>
#include <wayland-client.h>
#include <platform/wayland/core/Registry.hpp>
#include <platform/wayland/core/DisplayConnection.hpp>
#include <core/events/EventBus.hpp>
#include <thread>
#include <atomic>

using namespace luma::platform::wayland::core;
using namespace luma::core::events;

class MockCompositor {
public:
    MockCompositor() {
        display = wl_display_create();
        socket_name = wl_display_add_socket_auto(display);
        
        wl_global_create(display, &wl_compositor_interface, 4, nullptr, nullptr);
        wl_global_create(display, &wl_output_interface, 4, nullptr, nullptr);

        server_thread = std::thread([this]() {
            wl_display_run(display);
        });
    }
    
    ~MockCompositor() {
        wl_display_terminate(display);
        if (server_thread.joinable()) {
            server_thread.join();
        }
        wl_display_destroy(display);
    }

    wl_display* display;
    const char* socket_name;
    std::thread server_thread;
};

TEST(WaylandBackendTest, RegistryParsesGlobalsAndOutputs) {
    MockCompositor server;
    
    DisplayConnection client;
    auto res = client.connect(server.socket_name);
    ASSERT_TRUE(res.has_value());
    
    auto eventBus = std::make_shared<EventBus>();
    Registry registry(client.get(), eventBus);
    
    client.roundtrip(); 
    
    EXPECT_NE(registry.getCompositor(), nullptr);
}
