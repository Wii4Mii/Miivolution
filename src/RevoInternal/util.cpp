#include <string>
#include "Miivolution/database.hpp"
#include "util.hpp"


namespace {
miivolution::database::PrefPathConfig g_cfg;
std::filesystem::path g_cache;
constexpr const char* ORG_NAME = "Wii4Mii";
constexpr const char* LIB_NAME = "Miivolution";
} // namespace

namespace miivolution::database {
void setPrefPath(PrefPathConfig cfg) {
    if (cfg.get && !cfg.free) {
        throw std::invalid_argument(
            "miivolution: PrefPathConfig::get was provided without "
            "PrefPathConfig::free. The string returned by get() must be "
            "released by the same allocator that created it (e.g. SDL_free "
            "for SDL_GetPrefPath).");
    }
    g_cfg = std::move(cfg);
    g_cache.clear();
}
} // miivolution::database

namespace miivolution::util {
std::filesystem::path getPrefDir() {
    if (!g_cache.empty()) return g_cache;

    if (!g_cfg.get) {
        throw std::runtime_error(
            "miivolution: no pref path function set. Call "
            "miivolution::database::setPrefPath() before using the library. "
            "See Miivolution/database.hpp");
    }

    const std::unique_ptr<char, database::PrefFreeFn> raw{
        g_cfg.get(ORG_NAME, LIB_NAME), g_cfg.free};

    if (!raw || raw.get()[0] == '\0') {
        throw std::runtime_error(
            "miivolution: the pref path function failed to return a "
            "directory (it returned null or an empty string).");
    }

    g_cache = std::filesystem::path{
        std::u8string(reinterpret_cast<const char8_t *>(raw.get()))};
    return g_cache;
}

std::filesystem::path getFaceDatabase() {
    const auto dir = getPrefDir();
    return dir / "RFL_DB.dat";
}

} // miivolution::util
