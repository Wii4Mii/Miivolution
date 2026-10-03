#include "RFL_System.h"
#include "RFL_Database.h"
#include "RVLFaceLib/RFLi_Types.h"
#include "RVLFaceLib/RFLi_Database.h"

extern "C" {
    void RFLiConvertRaw2Info(const RFLiCharData* data, RFLiCharInfo* info);
    RFLiDatabase* RFLiGetDatabase();
}

extern "C" {

RFLErrcode RFLiPickupCharInfo(void* info, RFLDataSource source, RFLMiddleDB* db, u16 index) {
    if (!info) {
        return RFLErrcode_WrongParam;
    }

    if (!RFLAvailable()) {
        return RFLErrcode_NotAvailable;
    }

    auto* charInfo = static_cast<RFLiCharInfo*>(info);
    RFLErrcode err = RFLErrcode_Success;

    switch (source) {
    case RFLDataSource_Official:
        if (RFLIsAvailableOfficialData(index)) {
            RFLiDatabase* database = RFLiGetDatabase();
            if (database) {
                RFLiConvertRaw2Info(&database->rawData[index], charInfo);
                err = RFLErrcode_Success;
            } else {
                err = RFLErrcode_DBNodata;
            }
        } else {
            err = RFLErrcode_DBNodata;
        }
        break;
    case RFLDataSource_Controller1:
    case RFLDataSource_Controller2:
    case RFLDataSource_Controller3:
    case RFLDataSource_Controller4:
        err = RFLErrcode_Broken;
        break;
    case RFLDataSource_Middle:
        err = RFLErrcode_Broken;
        break;
    case RFLDataSource_Default:
        RFLiGetDefaultData(charInfo, index);
        err = RFLErrcode_Success;
        break;
    default:
        err = RFLErrcode_WrongParam;
        break;
    }

    return err;
}

}
