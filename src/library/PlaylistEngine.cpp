#include "PlaylistEngine.hpp"
#include <algorithm>

namespace luma::library {

PlaylistEngine::PlaylistEngine(std::shared_ptr<DatabaseManager> db)
    : m_db(std::move(db)) {
    std::random_device rd;
    m_rng = std::mt19937(rd());
}

PlaylistEngine::~PlaylistEngine() = default;

void PlaylistEngine::setMode(PlaylistMode mode) {
    m_mode = mode;
    if (m_mode == PlaylistMode::Shuffle) {
        rebuildShuffleIndices();
    }
}

void PlaylistEngine::loadAllWallpapers() {
    if (m_db) {
        setPlaylist(m_db->getAllWallpapers());
    }
}

void PlaylistEngine::setPlaylist(const std::vector<WallpaperRecord>& records) {
    m_playlist = records;
    m_currentIndex = 0;
    if (m_mode == PlaylistMode::Shuffle) {
        rebuildShuffleIndices();
    }
}

void PlaylistEngine::rebuildShuffleIndices() {
    m_shuffleIndices.clear();
    for (size_t i = 0; i < m_playlist.size(); ++i) {
        m_shuffleIndices.push_back(i);
    }
    std::shuffle(m_shuffleIndices.begin(), m_shuffleIndices.end(), m_rng);
    m_shufflePosition = 0;
}

bool PlaylistEngine::hasNext() const {
    return !m_playlist.empty();
}

WallpaperRecord PlaylistEngine::getNext() {
    if (m_playlist.empty()) return {};

    WallpaperRecord nextRecord;

    switch (m_mode) {
        case PlaylistMode::Sequential:
            nextRecord = m_playlist[m_currentIndex];
            m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
            break;

        case PlaylistMode::Random: {
            std::uniform_int_distribution<size_t> dist(0, m_playlist.size() - 1);
            m_currentIndex = dist(m_rng);
            nextRecord = m_playlist[m_currentIndex];
            break;
        }

        case PlaylistMode::Shuffle:
            if (m_shufflePosition >= m_shuffleIndices.size()) {
                rebuildShuffleIndices();
            }
            m_currentIndex = m_shuffleIndices[m_shufflePosition];
            nextRecord = m_playlist[m_currentIndex];
            m_shufflePosition++;
            break;
    }

    return nextRecord;
}

WallpaperRecord PlaylistEngine::getPrevious() {
    if (m_playlist.empty()) return {};

    switch (m_mode) {
        case PlaylistMode::Sequential:
            m_currentIndex = (m_currentIndex == 0) ? m_playlist.size() - 1 : m_currentIndex - 1;
            break;
        case PlaylistMode::Random:
        case PlaylistMode::Shuffle:
            // "Previous" in random/shuffle without history is just getting a new random or going back one in shuffle
            if (m_mode == PlaylistMode::Shuffle && m_shufflePosition > 1) {
                m_shufflePosition -= 2; 
            } else {
                // Not ideal, but just gets another one for random
            }
            return getNext();
    }

    return m_playlist[m_currentIndex];
}

} // namespace luma::library
