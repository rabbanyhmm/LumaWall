add_test([=[WaylandBackendTest.RegistryParsesGlobalsAndOutputs]=]  /home/rabbany/Desktop/LumaWall/build_release/src/tests/lumawall_wayland_tests [==[--gtest_filter=WaylandBackendTest.RegistryParsesGlobalsAndOutputs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WaylandBackendTest.RegistryParsesGlobalsAndOutputs]=]  PROPERTIES WORKING_DIRECTORY /home/rabbany/Desktop/LumaWall/build_release/src/tests SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  lumawall_wayland_tests_TESTS WaylandBackendTest.RegistryParsesGlobalsAndOutputs)
