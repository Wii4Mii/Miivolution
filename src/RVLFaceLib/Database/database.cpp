#include "RFL_Database.h"
#include "RFL_System.h"
#include "RVLFaceLib/RFLi_Database.h"
#include <cstring>

extern "C" {
    extern RFLiDatabase* RFLiGetDatabase();
    extern BOOL RFLiDBIsLoaded();
}

extern "C" {

BOOL RFLIsAvailableOfficialData(u16 index) {
    if (!RFLAvailable()) {
        return FALSE;
    }

    if (index >= RFL_DB_CHAR_MAX) {
        return FALSE;
    }

    RFLiDatabase* db = RFLiGetDatabase();
    if (!db) {
        return FALSE;
    }

    for (int i = 0; i < RFL_NAME_LEN; i++) {
        if (db->rawData[index].name[i] != 0) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL RFLSearchOfficialData(const RFLCreateID* id, u16* index) {
    if (!id || !index) {
        return FALSE;
    }

    if (!RFLAvailable()) {
        return FALSE;
    }

    RFLiDatabase* db = RFLiGetDatabase();
    if (!db) {
        return FALSE;
    }

    for (u32 i = 0; i < RFL_DB_CHAR_MAX; i++) {
        if (std::memcmp(&db->rawData[i].createID, id, sizeof(RFLCreateID)) == 0) {
            bool hasName = false;
            for (int j = 0; j < RFL_NAME_LEN; j++) {
                if (db->rawData[i].name[j] != 0) {
                    hasName = true;
                    break;
                }
            }

            if (hasName) {
                *index = static_cast<u16>(i);
                return TRUE;
            }
        }
    }

    return FALSE;
}

}
