/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SoftwareRenderer"
#include <utils/Log.h>

#include "../include/SoftwareRenderer.h"

#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapPmem.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#include <ui/GraphicBufferMapper.h>

namespace android {

SoftwareRenderer::SoftwareRenderer(
        const sp<Surface> &surface, const sp<MetaData> &meta)
    : mConverter(NULL),
      mYUVMode(None),
      mSurface(surface) {
    int32_t tmp;
    CHECK(meta->findInt32(kKeyColorFormat, &tmp));
    mColorFormat = (OMX_COLOR_FORMATTYPE)tmp;

    CHECK(meta->findInt32(kKeyWidth, &mWidth));
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    if (!meta->findRect(
                kKeyCropRect,
                &mCropLeft, &mCropTop, &mCropRight, &mCropBottom)) {
        mCropLeft = mCropTop = 0;
        mCropRight = mWidth - 1;
        mCropBottom = mHeight - 1;
    }

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    int halFormat;
    switch (mColorFormat) {
        default:
            halFormat = HAL_PIXEL_FORMAT_RGB_565;

            mConverter = new ColorConverter(
                    mColorFormat, OMX_COLOR_Format16bitRGB565);
            CHECK(mConverter->isValid());
            break;
    }

    CHECK(mSurface.get() != NULL);
    CHECK(mWidth > 0);
    CHECK(mHeight > 0);
    CHECK(mConverter == NULL || mConverter->isValid());

    CHECK_EQ(0,
            native_window_set_usage(
            mSurface.get(),
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0, native_window_set_buffer_count(mSurface.get(), 2));

    // Width must be multiple of 32???
    CHECK_EQ(0, native_window_set_buffers_geometry(
                mSurface.get(),
                mCropRight - mCropLeft + 1,
                mCropBottom - mCropTop + 1,
                halFormat));

    uint32_t transform;
    switch (rotationDegrees) {
        case 0: transform = 0; break;
        case 90: transform = HAL_TRANSFORM_ROT_90; break;
        case 180: transform = HAL_TRANSFORM_ROT_180; break;
        case 270: transform = HAL_TRANSFORM_ROT_270; break;
        default: transform = 0; break;
    }

    if (transform) {
        CHECK_EQ(0, native_window_set_buffers_transform(
                    mSurface.get(), transform));
    }
}

SoftwareRenderer::~SoftwareRenderer() {
    delete mConverter;
    mConverter = NULL;
}

void SoftwareRenderer::render(
        const void *data, size_t size, void *platformPrivate) {
    android_native_buffer_t *buf;
    int err;
    if ((err = mSurface->dequeueBuffer(mSurface.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }

    CHECK_EQ(0, mSurface->lockBuffer(mSurface.get(), buf));

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    Rect bounds(mWidth, mHeight);

    void *dst;
    CHECK_EQ(0, mapper.lock(
                buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

    if (mConverter) {
        mConverter->convert(
                data,
                mWidth, mHeight,
                mCropLeft, mCropTop, mCropRight, mCropBottom,
                dst,
                buf->stride, buf->height,
                0, 0,
                mCropRight - mCropLeft,
                mCropBottom - mCropTop);
    } else {
        TRESPASS();
    }

    CHECK_EQ(0, mapper.unlock(buf->handle));

    if ((err = mSurface->queueBuffer(mSurface.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
}

}  // namespace android
