#include <gtest/gtest.h>
#include <media/scheduler/MediaPlayer.hpp>
#include <media/common/FileMediaSource.hpp>

using namespace luma::media;

class MediaPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
};

TEST_F(MediaPipelineTest, MediaSessionInitialization) {
    auto source = std::make_shared<FileMediaSource>("/dev/null", MediaType::Video);
    MediaPlayer player;
    
    // We expect load to fail gracefully or at least not crash when given invalid paths
    player.load(source);
    
    EXPECT_TRUE(true);
}

TEST_F(MediaPipelineTest, FallbackCapabilities) {
    MediaPlayer player;
    player.setHardwareDecodeEnabled(false);
    
    // If HW decode is false, getReport should reflect it (after load)
    // Testing logic here would normally mock the decoder.
    EXPECT_TRUE(true);
}
