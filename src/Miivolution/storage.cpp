#include "Miivolution/storage.hpp"
#include "Miivolution/database.hpp"
#include "RevoInternal/util.hpp"
#include <fstream>

#define MII_RAW_SIZE 0x4A

namespace {

std::filesystem::path getMiixportsDir() {
    const auto prefDir = miivolution::util::getPrefDir();
    return prefDir / "miixports";
}

std::filesystem::path getMiimportsDir() {
    const auto prefDir = miivolution::util::getPrefDir();
    return prefDir / "miimports";
}

std::string getMiiFileName(const miivolution::mii::MII_DATA_STRUCT& mii) {
    std::string name;
    for (int i = 0; i < RFL_NAME_LEN && mii.name[i] != 0; ++i) {
        if (mii.name[i] < 128) {
            name += static_cast<char>(mii.name[i]);
        } else {
            name += '_';
        }
    }

    if (name.empty()) {
        name = "unnamed";
    }

    char createIDHex[17];
    snprintf(createIDHex, sizeof(createIDHex), "%02x%02x%02x%02x%02x%02x%02x%02x",
             mii.createID.data[0], mii.createID.data[1], mii.createID.data[2], mii.createID.data[3],
             mii.createID.data[4], mii.createID.data[5], mii.createID.data[6], mii.createID.data[7]);

    return name + "_" + std::string(createIDHex) + ".mii";
}

std::filesystem::path resolveExportPath(const std::optional<std::filesystem::path>& path,
                                       const miivolution::mii::MII_DATA_STRUCT& mii) {
    if (path.has_value()) {
        return path.value();
    }

    const auto miisDir = getMiixportsDir();
    std::filesystem::create_directories(miisDir);
    return miisDir / getMiiFileName(mii);
}

}

namespace miivolution::storage {

bool exportMii(const mii::MII_DATA_STRUCT& mii,
               const std::optional<std::filesystem::path>& path) {
    const auto filePath = resolveExportPath(path, mii);

    u8 buffer[MII_RAW_SIZE];
    if (!mii::serializeMii(mii, buffer, MII_RAW_SIZE)) {
        return false;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(buffer), MII_RAW_SIZE);
    return file.good();
}

bool exportMii(u16 index, const std::optional<std::filesystem::path>& path) {
    mii::MII_DATA_STRUCT mii;
    if (!database::getMii(index, mii)) {
        return false;
    }

    return exportMii(mii, path);
}

bool exportMii(const RFLCreateID& createID,
               const std::optional<std::filesystem::path>& path) {
    const s32 index = database::findMiiByCreateID(createID);
    if (index < 0) {
        return false;
    }

    return exportMii(static_cast<u16>(index), path);
}

bool importMii(const std::filesystem::path& path, mii::MII_DATA_STRUCT& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    u8 buffer[MII_RAW_SIZE];
    file.read(reinterpret_cast<char*>(buffer), MII_RAW_SIZE);

    if (!file || file.gcount() != MII_RAW_SIZE) {
        return false;
    }

    return mii::deserializeMii(buffer, MII_RAW_SIZE, out);
}

bool importMiiToDB(const std::filesystem::path& path, u16* outIndex) {
    mii::MII_DATA_STRUCT mii;
    if (!importMii(path, mii)) {
        return false;
    }

    s32 existingIndex = database::findMiiByCreateID(mii.createID);
    if (existingIndex >= 0) {
        if (outIndex) {
            *outIndex = static_cast<u16>(existingIndex);
        }
        return false;
    }

    return database::addMii(mii, outIndex);
}

std::vector<std::filesystem::path> getMiixports() {
    std::vector<std::filesystem::path> files;
    const auto dir = getMiixportsDir();

    if (!std::filesystem::exists(dir)) {
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".mii" || entry.path().extension() == ".rcd")) {
            files.push_back(entry.path());
        }
    }

    return files;
}

std::vector<std::filesystem::path> getMiimports() {
    std::vector<std::filesystem::path> files;
    const auto dir = getMiimportsDir();

    if (!std::filesystem::exists(dir)) {
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mii") {
            files.push_back(entry.path());
        }
    }

    return files;
}

u32 importAllMiis(std::vector<u16>* outIndices) {
    const auto files = getMiimports();
    u32 imported = 0;

    for (const auto& file : files) {
        u16 index;
        if (importMiiToDB(file, &index)) {
            imported++;
            if (outIndices) {
                outIndices->push_back(index);
            }
        }
    }

    return imported;
}

}
