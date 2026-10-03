#include <catch2/catch_test_macros.hpp>
#include "RVLFaceLib.h"
#include "RVLFaceLib/RFLi_Types.h"
#include <vector>

namespace {
    struct RFLTestFixture {
        RFLTestFixture() {
            u32 workSize = RFLGetWorkSize(FALSE);
            u32 resSize = 1024 * 1024;

            workBuffer.resize(workSize);
            resBuffer.resize(resSize);

            RFLInitRes(workBuffer.data(), resBuffer.data(), resSize, FALSE);
        }

        ~RFLTestFixture() {
            RFLExit();
        }

        std::vector<u8> workBuffer;
        std::vector<u8> resBuffer;
    };
}

TEST_CASE("Default Mii database exists", "[rfl][default-db]") {
    RFLTestFixture fixture;
    REQUIRE(RFLAvailable() == TRUE);
}

TEST_CASE("Default Miis can be loaded", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLiCharInfo charInfo;
    RFLMiddleDB* db = nullptr;

    for (u16 i = 0; i < 6; i++) {
        RFLErrcode err = RFLiPickupCharInfo(&charInfo, RFLDataSource_Default, db, i);
        REQUIRE(err == RFLErrcode_Success);
    }
}

TEST_CASE("Default Mii Guest A has expected data", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLiCharInfo charInfo;
    RFLMiddleDB* db = nullptr;

    RFLErrcode err = RFLiPickupCharInfo(&charInfo, RFLDataSource_Default, db, 0);
    REQUIRE(err == RFLErrcode_Success);

    REQUIRE(charInfo.body.height == 64);
    REQUIRE(charInfo.body.build == 64);
    REQUIRE(charInfo.personal.sex == 0);
    REQUIRE(charInfo.personal.color == 4);
}

TEST_CASE("Default Mii Guest B has different data than Guest A", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLiCharInfo charInfoA, charInfoB;
    RFLMiddleDB* db = nullptr;

    RFLErrcode errA = RFLiPickupCharInfo(&charInfoA, RFLDataSource_Default, db, 0);
    RFLErrcode errB = RFLiPickupCharInfo(&charInfoB, RFLDataSource_Default, db, 1);

    REQUIRE(errA == RFLErrcode_Success);
    REQUIRE(errB == RFLErrcode_Success);

    REQUIRE(charInfoA.hair.rawdata != charInfoB.hair.rawdata);
}

TEST_CASE("Default Mii indices wrap around correctly", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLMiddleDB* db = nullptr;

    RFLiCharInfo charInfo6, charInfo12;
    RFLErrcode err6 = RFLiPickupCharInfo(&charInfo6, RFLDataSource_Default, db, 6);
    RFLErrcode err12 = RFLiPickupCharInfo(&charInfo12, RFLDataSource_Default, db, 12);

    REQUIRE(err6 == RFLErrcode_Success);
    REQUIRE(err12 == RFLErrcode_Success);

    REQUIRE(charInfo6.body.height == charInfo12.body.height);
    REQUIRE(charInfo6.body.build == charInfo12.body.build);
}

TEST_CASE("Invalid data sources return correct errors", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLiCharInfo charInfo;
    RFLMiddleDB* db = nullptr;

    // official data source fails with empty db (no data at index 0)
    RFLErrcode err = RFLiPickupCharInfo(&charInfo, RFLDataSource_Official, db, 0);
    REQUIRE(err == RFLErrcode_DBNodata);

    // controller data sources fail with null db (require actual controller data)
    err = RFLiPickupCharInfo(&charInfo, RFLDataSource_Controller1, db, 0);
    REQUIRE(err == RFLErrcode_Broken);
}

TEST_CASE("Bitfield structure layouts are correct", "[rfl][default-db]") {
    RFLTestFixture fixture;
    RFLiCharInfo charInfo;
    RFLMiddleDB* db = nullptr;

    RFLErrcode err = RFLiPickupCharInfo(&charInfo, RFLDataSource_Default, db, 0);
    REQUIRE(err == RFLErrcode_Success);

    REQUIRE(charInfo.faceline.type < 8);
    REQUIRE(charInfo.faceline.color < 8);
    REQUIRE(charInfo.faceline.texture < 16);

    REQUIRE(charInfo.hair.type < 128);
    REQUIRE(charInfo.hair.color < 8);
    REQUIRE(charInfo.hair.flip <= 1);
}
