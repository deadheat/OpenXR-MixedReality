#include <bgfx/bgfx.h>

#include <bgfx/platform.h>

#include <bx/uint32_t.h>

template <typename HandleType>
class UniqueBgfxHandle {
public:
    UniqueBgfxHandle() = default;
    explicit operator bool() const noexcept;
    explicit UniqueBgfxHandle(HandleType handle)
        : m_handle(handle) {
    }
    UniqueBgfxHandle(const UniqueBgfxHandle&) = delete;
    UniqueBgfxHandle(UniqueBgfxHandle&& other) noexcept;

    ~UniqueBgfxHandle() noexcept {
        Reset();
    }

    UniqueBgfxHandle& operator=(const UniqueBgfxHandle&) = delete;
    UniqueBgfxHandle& operator=(UniqueBgfxHandle&& other) noexcept;

    HandleType Get() const noexcept;

    HandleType* Put() noexcept;

    void Reset() noexcept;

private:
    HandleType m_handle{bgfx::kInvalidHandle};
};

template <typename HandleType2>
class SharedBgfxHandle {
public:
    SharedBgfxHandle() = default;
    explicit operator bool() const noexcept;
    explicit SharedBgfxHandle(HandleType2 handle);
    explicit SharedBgfxHandle(HandleType2 handle, int* refcount);
    SharedBgfxHandle(const SharedBgfxHandle&);
    SharedBgfxHandle(SharedBgfxHandle&& other) noexcept;

    ~SharedBgfxHandle() noexcept;

    SharedBgfxHandle& operator=(const SharedBgfxHandle&);

    SharedBgfxHandle& operator=(SharedBgfxHandle&& other) noexcept;

    SharedBgfxHandle<HandleType2> Copy() const noexcept;
    HandleType2 Get() const noexcept;
    
    HandleType2* Put() noexcept;

    void Reset() noexcept;

private:
    int* m_refcount;
    HandleType2 m_handle{bgfx::kInvalidHandle};
};
