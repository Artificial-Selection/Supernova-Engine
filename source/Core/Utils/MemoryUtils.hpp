//
// Created by Devilast on 7/11/2021.
//
#pragma once

#include <memory>

namespace snv
{
    __forceinline void MemZero(void* buf, size_t size)
    {
        std::memset(buf, 0, size);
    }
}