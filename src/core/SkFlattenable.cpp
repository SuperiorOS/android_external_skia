/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkFlattenable.h"
#include "SkPtrRecorder.h"
#include "SkReadBuffer.h"

SkNamedFactorySet::SkNamedFactorySet() : fNextAddedFactory(0) {}

uint32_t SkNamedFactorySet::find(SkFlattenable::Factory factory) {
    uint32_t index = fFactorySet.find(factory);
    if (index > 0) {
        return index;
    }
    const char* name = SkFlattenable::FactoryToName(factory);
    if (nullptr == name) {
        return 0;
    }
    *fNames.append() = name;
    return fFactorySet.add(factory);
}

const char* SkNamedFactorySet::getNextAddedFactoryName() {
    if (fNextAddedFactory < fNames.count()) {
        return fNames[fNextAddedFactory++];
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

SkRefCntSet::~SkRefCntSet() {
    // call this now, while our decPtr() is sill in scope
    this->reset();
}

void SkRefCntSet::incPtr(void* ptr) {
    ((SkRefCnt*)ptr)->ref();
}

void SkRefCntSet::decPtr(void* ptr) {
    ((SkRefCnt*)ptr)->unref();
}

///////////////////////////////////////////////////////////////////////////////

struct Entry {
    const char*             fName;
    SkFlattenable::Factory  fFactory;
    SkFlattenable::Type     fType;
};

static int gCount = 0;
static Entry gEntries[128];

void SkFlattenable::Register(const char name[], Factory factory, SkFlattenable::Type type) {
    SkASSERT(name);
    SkASSERT(factory);
    SkASSERT(gCount < (int)SK_ARRAY_COUNT(gEntries));

    gEntries[gCount].fName = name;
    gEntries[gCount].fFactory = factory;
    gEntries[gCount].fType = type;
    gCount += 1;
}

#ifdef SK_DEBUG
static void report_no_entries(const char* functionName) {
    if (!gCount) {
        SkDebugf("%s has no registered name/factory/type entries."
                 " Call SkFlattenable::InitializeFlattenablesIfNeeded() before using gEntries",
                 functionName);
    }
}
#endif

SkFlattenable::Factory SkFlattenable::NameToFactory(const char name[]) {
    InitializeFlattenablesIfNeeded();
#ifdef SK_DEBUG
    report_no_entries(__FUNCTION__);
#endif
    const Entry* entries = gEntries;
    for (int i = gCount - 1; i >= 0; --i) {
        if (strcmp(entries[i].fName, name) == 0) {
            return entries[i].fFactory;
        }
    }
    return nullptr;
}

bool SkFlattenable::NameToType(const char name[], SkFlattenable::Type* type) {
    SkASSERT(type);
    InitializeFlattenablesIfNeeded();
#ifdef SK_DEBUG
    report_no_entries(__FUNCTION__);
#endif
    const Entry* entries = gEntries;
    for (int i = gCount - 1; i >= 0; --i) {
        if (strcmp(entries[i].fName, name) == 0) {
            *type = entries[i].fType;
            return true;
        }
    }
    return false;
}

const char* SkFlattenable::FactoryToName(Factory fact) {
    InitializeFlattenablesIfNeeded();
#ifdef SK_DEBUG
    report_no_entries(__FUNCTION__);
#endif
    const Entry* entries = gEntries;
    for (int i = gCount - 1; i >= 0; --i) {
        if (entries[i].fFactory == fact) {
            return entries[i].fName;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkData> SkFlattenable::serialize(const SkSerialProcs* procs) const {
    SkBinaryWriteBuffer writer;
    if (procs) {
        writer.setSerialProcs(*procs);
    }
    writer.writeFlattenable(this);
    size_t size = writer.bytesWritten();
    auto data = SkData::MakeUninitialized(size);
    writer.writeToMemory(data->writable_data());
    return data;
}

sk_sp<SkFlattenable> SkFlattenable::Deserialize(SkFlattenable::Type type, const void* data,
                                                size_t size, const SkDeserialProcs* procs) {
    SkReadBuffer buffer(data, size);
    if (procs) {
        buffer.setDeserialProcs(*procs);
    }
    return sk_sp<SkFlattenable>(buffer.readFlattenable(type));
}
