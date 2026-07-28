#pragma once

#include <vector>
#include <memory>
#include "DatabaseManager.hpp"
#include <random>

namespace luma::library {

enum class PlaylistMode {
    Sequential,
    Random,
    Shuffle
};

class PlaylistEngine {
public:
    PlaylistEngine(std::shared_ptr<DatabaseManager> db);
    ~PlaylistEngine();

    void setMode(PlaylistMode mode);
    void loadAllWallpapers();
    void setPlaylist(const std::vector<WallpaperRecord>& records);

    bool hasNext() const;
    WallpaperRecord getNext();
    WallpaperRecord getPrevious();

private:
    std::shared_ptr<DatabaseManager> m_db;
    std::vector<WallpaperRecord> m_playlist;
    
    PlaylistMode m_mode{PlaylistMode::Sequential};
    size_t m_currentIndex{0};
    std::vector<size_t> m_shuffleIndices;
    size_t m_shufflePosition{0};
    std::mt19937 m_rng;

    void rebuildShuffleIndices();
};

} // namespace luma::library
