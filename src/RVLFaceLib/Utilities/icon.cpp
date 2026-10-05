#include <cmath>
#include <numbers>
#include <cstring>
#include <bit>

#include "RFL_Icon.h"
#include "RFL_Model.h"
#include "RVLFaceLib/RFLi_Types.h"
#include "RVLFaceLib/Model/model_internal.hpp"

#if DOLPHIN_INCLUDES
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#else
#include <revolution/gx.h>
#include <revolution/mtx.h>
#endif

using namespace rvlfacelib;

namespace {
RFLCallback iconDrawCallback;
CoordData defaultCoordData = {1, 2, 0, FALSE, FALSE, FALSE};
CoordData* coordinateData = &defaultCoordData;

constexpr u32 roundUp(u32 value, u32 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

constexpr u32 getMaskSize(u32 resolution) {
    return 2 * (resolution * resolution);
}

constexpr u32 getMaskBufSize(RFLResolution resolution) {
    u32 size = 0;
    u32 res = static_cast<u32>(resolution);

    if (res & 32) size += getMaskSize(32);
    if (res & 64) size += getMaskSize(64);
    if (res & 128) size += getMaskSize(128);
    if (res & 256) size += getMaskSize(256);

    return size;
}

constexpr u32 countExpressions(u32 exprFlags) {
    return static_cast<u32>(std::popcount(exprFlags));
}

void convertCharInfo(const RFLiCharInfo& in, CharInfo* out) {
    std::memset(out, 0, sizeof(*out));
    out->facelineType = in.faceline.type;
    out->facelineColor = in.faceline.color;
    out->facelineTexture = in.faceline.texture;
    out->hairType = in.hair.type;
    out->hairColor = in.hair.color;
    out->hairFlip = in.hair.flip;
    out->noseType = in.nose.type;
    out->noseScale = in.nose.scale;
    out->noseY = in.nose.y;
    out->beardType = in.beard.type;
    out->beardColor = in.beard.color;
    out->beardScale = in.beard.scale;
    out->beardY = in.beard.y;
    out->beardMustache = in.beard.mustache;
    out->glassType = in.glass.type;
    out->glassColor = in.glass.color;
    out->glassScale = in.glass.scale;
    out->glassY = in.glass.y;
    out->personalColor = in.personal.color;
}

}

extern "C" {
    RFLErrcode RFLiPickupCharInfo(void* info, RFLDataSource source, RFLMiddleDB* db, u16 index);
    BOOL RFLiGetUseDeluxTex();
    void* RFLiAlloc32(u32 size);
    void RFLiFree(void* block);
}

namespace {

void RFLiInitCharModel(RFLCharModel* model, RFLiCharInfo* info, void* work, RFLResolution res, u32 exprFlags) {
    std::memset(model, 0, sizeof(RFLCharModel));

    u8* workPtr = static_cast<u8*>(work);
    auto* internal = reinterpret_cast<CharModelInternal*>(workPtr);
    workPtr += roundUp(sizeof(CharModelInternal), 32);

    *reinterpret_cast<CharModelInternal**>(model) = internal;

    std::memset(internal, 0, sizeof(CharModelInternal));
    internal->currentExpression = RFLExp_Normal;
    internal->resolution = res;

    const u32 exprNum = countExpressions(exprFlags);
    auto* exprTexObj = reinterpret_cast<GXTexObj*>(workPtr);
    workPtr += roundUp(exprNum * sizeof(GXTexObj), 32);

    for (u32 i = 0; i < RFLExp_Max; i++) {
        if (exprFlags & (1 << i)) {
            internal->maskTexObj[i] = exprTexObj;
            exprTexObj++;
        } else {
            internal->maskTexObj[i] = nullptr;
        }
    }

    internal->res = reinterpret_cast<CharModelRes*>(workPtr);
    std::memset(internal->res, 0, sizeof(CharModelRes));
    workPtr += roundUp(sizeof(CharModelRes), 32);

    CharInfo ci;
    convertCharInfo(*info, &ci);
    RFLiInitCharModelRes(internal->res, &ci);
    if (info->glass.type == 0) {
        internal->res->glassesDlSize = 0;
    }

    int topRes = 64;
    u32 resBits = static_cast<u32>(res);
    if (resBits & 256) topRes = 256;
    else if (resBits & 128) topRes = 128;
    const u32 maskStride = getMaskBufSize(res);
    for (u32 i = 0; i < RFLExp_Max; i++) {
        if (!internal->maskTexObj[i]) continue;
        composeMask(workPtr, topRes, *info, i == RFLExp_Blink);
        GXInitTexObj(internal->maskTexObj[i], workPtr, topRes, topRes, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(internal->maskTexObj[i], GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        workPtr += roundUp(maskStride, 32);
    }
}

}

void RFLiSetupCopyTex(GXTexFmt fmt, u16 width, u16 height, void* buf, GXColor clearColor) {
    GXSetFog(GX_FOG_NONE, 1.0f, 1.0f, 0.0f, 0.0f, GXColor{0, 0, 0, 0});
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetDstAlpha(GX_FALSE, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
    GXSetCopyFilter(GX_FALSE, nullptr, GX_FALSE, nullptr);
#ifdef AURORA
    GXSetCopyClamp(static_cast<GXFBClamp>(GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
#else
    GXSetCopyClamp(static_cast<GXClamp>(GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
#endif
    GXSetCopyClear(clearColor, GX_MAX_Z24);
    GXSetTexCopySrc(0, 0, width, height);
    GXSetTexCopyDst(width, height, fmt, GX_FALSE);
    GXCopyTex(buf, GX_TRUE);
    GXPixModeSync();
}

void RFLiMakeIcon(void* buf, RFLiCharInfo* info, RFLExpression expression, const RFLIconSetting* setting) {
    RFLCharModel model;
    void* modelBuf;
    f32 vp[6];
    u32 byteSize;
    Mtx viewMtx;
    u32 scissorOffsetX;
    u32 scissorOffsetY;
    u32 scissorWidth;
    u32 scissorHeight;
    CoordData iconCoordData;
    CoordData coordData;
    u32 bufSize;
    RFLResolution resolution;
    RFLExpFlag expFlag;
    GXColor backColor;
    Mtx44 projMtx;
    f32 fovy;
    f32 aspect;
    Vec cameraPos;
    Vec target;
    Vec cameraUp;
    GXLightObj light;
    Vec pos;
    RFLDrawSetting drawSetting;

    iconCoordData = CoordData{1, 2, 0, FALSE, FALSE, FALSE};
    byteSize = setting->width * setting->height * sizeof(u16);
    coordData = *coordinateData;
    coordinateData = &iconCoordData;

    if ((setting->width > 128 || setting->height > 128) && RFLiGetUseDeluxTex()) {
        resolution = RFLResolution_256;
    } else if (setting->width > 64 || setting->height > 64) {
        resolution = RFLResolution_128;
    } else {
        resolution = RFLResolution_64;
    }

    expFlag = static_cast<RFLExpFlag>(1 << expression);
    bufSize = RFLGetModelBufferSize(resolution, expFlag);
    modelBuf = RFLiAlloc32(bufSize);
    RFLiInitCharModel(&model, info, modelBuf, resolution, expFlag);
    RFLSetExpression(&model, expression);

    if (setting->bgType == RFLIconBG_Direct) {
        backColor = setting->bgColor;
    } else {
        backColor = RFLGetFavoriteColor(static_cast<RFLFavoriteColor>(info->personal.color));
    }
    backColor.a = 0;

    GXGetScissor(&scissorOffsetX, &scissorOffsetY, &scissorWidth, &scissorHeight);
    GXSetScissor(0, 0, setting->width, setting->height);

    RFLiSetupCopyTex(static_cast<GXTexFmt>(GX_RGBA8), setting->width, setting->height, buf, backColor);

    GXGetViewportv(vp);
    GXSetViewport(0.0f, 0.0f, setting->width, setting->height, 0.0f, 1.0f);

    aspect = (f32)setting->width / (f32)setting->height;
    if (setting->width < setting->height) {
        fovy = 2 * ((180.0f / std::numbers::pi_v<float>) *
                    std::atan2(43.2f / aspect, 500.0f));
    } else {
        fovy = 2.0f * (180.0f / std::numbers::pi_v<float>) *
            std::atan2(43.2f, 500.0f);
    }

    C_MTXPerspective(projMtx, fovy, aspect, 500.0f, 700.0f);
    GXSetProjection(projMtx, GX_PERSPECTIVE);

    cameraPos = Vec{0.0f, 34.5f, 600.0f};
    target = Vec{0.0f, 34.5f, 0.0f};
    cameraUp = Vec{0.0f, 1.0f, 0.0f};

    C_MTXLookAt(viewMtx, &cameraPos, &cameraUp, &target);
    GXInitLightColor(&light, GXColor{255, 255, 255, 255});

    pos = Vec{1600.0f, 1500.0f, 6000.0f};

    PSMTXMultVec(viewMtx, &pos, &pos);
    GXInitLightPos(&light, pos.x, pos.y, pos.z);
    GXLoadLightObjImm(&light, GX_LIGHT0);
    RFLSetMtx(&model, viewMtx);

    drawSetting.lightEnable = TRUE;
    drawSetting.lightMask = GX_LIGHT0;
    drawSetting.diffuse = GX_DF_CLAMP;
    drawSetting.attn = GX_AF_NONE;
    drawSetting.ambColor = GXColor{160, 160, 160, 255};
    drawSetting.compLoc = 0;
    RFLLoadDrawSetting(&drawSetting);

    if (!setting->drawXluOnly) {
        GXSetColorUpdate(TRUE);
        GXSetAlphaUpdate(TRUE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_COPY);
        RFLDrawOpa(&model);
    }

    GXSetZMode(TRUE, GX_LEQUAL, FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_COPY);
    GXSetColorUpdate(TRUE);
    GXSetAlphaUpdate(FALSE);
    RFLDrawXlu(&model);

    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_INVSRCALPHA, GX_LO_COPY);
    GXSetAlphaUpdate(TRUE);
    GXSetColorUpdate(FALSE);
    RFLDrawXlu(&model);

    GXSetZMode(TRUE, GX_LEQUAL, TRUE);
    GXSetColorUpdate(TRUE);
    GXCopyTex(buf, TRUE);
    GXPixModeSync();

    if (iconDrawCallback == NULL) {
        GXDrawDone();
    } else {
        iconDrawCallback();
    }

    RFLiFree(modelBuf);
    GXSetViewport(vp[0], vp[1], vp[2], vp[3], vp[4], vp[5]);
    GXSetScissor(scissorOffsetX, scissorOffsetY, scissorWidth, scissorHeight);
    coordinateData = &coordData;
}

RFLErrcode RFLMakeIcon(void* buf, RFLDataSource source, RFLMiddleDB* middleDB,
                       u16 index, RFLExpression expression,
                       const RFLIconSetting* setting) {
    RFLiCharInfo info;
    RFLErrcode err = RFLiPickupCharInfo(&info, source, middleDB, index);

    if (err == RFLErrcode_Success) {
        RFLiMakeIcon(buf, &info, expression, setting);
    }

    return err;
}

void RFLSetIconDrawDoneCallback(RFLCallback callback) {
    iconDrawCallback = callback;
}
