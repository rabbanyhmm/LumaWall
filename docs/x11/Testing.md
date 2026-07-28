# X11 Testing Strategy

Testing an X11 client natively requires a functional X server. Unlike Wayland where we can easily spin up an in-memory `wl_display` thread for isolated mocking, mocking XCB at the socket level is complex and brittle.

## Xvfb Integration
We run the `GoogleTest` suite under `Xvfb` (X virtual framebuffer). This allows our integration tests to:
- Establish real XCB connections.
- Verify RandR extension availability.
- Create and map real `xcb_window_t` elements.
- Validate our RAII lifecycle and atom cache.

To run the test suite:
```bash
xvfb-run -s "-screen 0 1920x1080x24" ./build/src/tests/lumawall_x11_tests
```
