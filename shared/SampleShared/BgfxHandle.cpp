#include "pch.h"
#include "BgfxHandle.h"

template <typename HandleType>
class UniqueBgfxHandle {
    explicit operator bool() const noexcept {
        return m_handle != bgfx::kInvalidHandle;
    }
    UniqueBgfxHandle::UniqueBgfxHandle(UniqueBgfxHandle&& other) noexcept {
        *this = std::move(other);
    }

    ~UniqueBgfxHandle() noexcept {
        Reset();
    }
    UniqueBgfxHandle& UniqueBgfxHandle::operator=(UniqueBgfxHandle&& other) noexcept {
        if (m_handle.idx != other.m_handle.idx) {
            Reset();

            m_handle = other.m_handle;
            other.m_handle = {bgfx::kInvalidHandle};
        }
        return *this;
    }

    HandleType UniqueBgfxHandle::Get() const noexcept {
        return m_handle;
    }

    HandleType* UniqueBgfxHandle::Put() noexcept {
        Reset();
        return &m_handle;
    }

    void UniqueBgfxHandle::Reset() noexcept {
        if (bgfx::isValid(m_handle)) {
            bgfx::destroy(m_handle);
            m_handle = {bgfx::kInvalidHandle};
        }
    }
};



template <typename HandleType2>
class SharedBgfxHandle {
    // SharedBgfxHandle() = default;
    explicit SharedBgfxHandle::SharedBgfxHandle(HandleType2 handle) {
        m_handle = handle;
    }
    explicit SharedBgfxHandle::operator bool() const noexcept {
        return m_handle != bgfx::kInvalidHandle;
    }
    explicit SharedBgfxHandle::SharedBgfxHandle(HandleType2 handle) {
        m_handle = handle;
        *m_refcount = 0;
    }
    explicit SharedBgfxHandle::SharedBgfxHandle(HandleType2 handle, int* refcount) {
        m_handle = handle;
        *refcount = *refcount + 1;
        m_refcount = refcount;
    }

    SharedBgfxHandle<HandleType2>::SharedBgfxHandle(const UniqueBgfxHandle&) {
        return this.Copy();
    }
    // SharedBgfxHandle(const SharedBgfxHandle&) = delete;
    SharedBgfxHandle::SharedBgfxHandle(SharedBgfxHandle&& other) noexcept {
        *this = std::move(other);
    }

    ~SharedBgfxHandle() noexcept {
        if (*m_refcount < 1) {
            Reset();
        } else {
            *m_refcount = *m_refcount - 1;
        }
    }
    SharedBgfxHandle& SharedBgfxHandle<HandleType2>::operator=(const SharedBgfxHandle&) {
        return this.Copy();
    }

    // SharedBgfxHandle& operator=(const SharedBgfxHandle&) = delete;
    SharedBgfxHandle& SharedBgfxHandle::operator=(SharedBgfxHandle&& other) noexcept {
        if (m_handle.idx != other.m_handle.idx) {
            Reset();

            m_handle = other.m_handle;
            other.m_handle = {bgfx::kInvalidHandle};
        }
        return *this;
    }

    HandleType2 SharedBgfxHandle::Get() const noexcept {
        return m_handle;
    }

    HandleType2* SharedBgfxHandle::Put() noexcept {
        Reset();
        return &m_handle;
    }
    SharedBgfxHandle<HandleType2> SharedBgfxHandle::SharedBgfxHandle::Copy() const noexcept {
        return SharedBgfxHandle<HandleType2>(m_handle, m_refcount);
    }

    void Reset() noexcept {
        if (bgfx::isValid(m_handle)) {
            bgfx::destroy(m_handle);
            m_handle = {bgfx::kInvalidHandle};
        }
    }
};


