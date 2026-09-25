#include <dlfcn.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
namespace android {
    struct Rect { int32_t left, top, right, bottom; };
    struct native_handle;
}

extern "C" {
    // Android 11 removed GraphicBufferMapper::lock(..., int*, int*). We forward it to the modern 4-argument lock.
    __attribute__((visibility("default")))
    int _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPvPiS9_(
            void* mapper_this, 
            const android::native_handle* handle, 
            unsigned int usage, 
            const android::Rect& bounds, 
            void** vaddr, 
            int* outBytesPerPixel, 
            int* outBytesPerStride) {
        
        // The legacy API allowed output pointers for bytes-per-pixel and bytes-per-stride.
        // We cannot safely provide these outputs using the modern 4-arg lock API, but returning
        // -EINVAL breaks decoders that pass non-null pointers. We must ignore them instead.

        static void* libui = []() { void* h = dlopen("libui-v32.so", RTLD_NOW); return h ? h : dlopen("libui.so", RTLD_NOW); }();
        // Modern 4-argument lock: GraphicBufferMapper::lock(native_handle const*, uint64_t, Rect const&, void**)
        static void* sym = libui ? dlsym(libui, "_ZN7android19GraphicBufferMapper4lockEPK13native_handleyRKNS_4RectEPPv") : nullptr;
        
        int err = -1; // Fallback to error if symbol not found
        if (sym) {
            typedef int (*LockFunc)(void*, const android::native_handle*, uint64_t, const android::Rect&, void**);
            LockFunc lock_func = (LockFunc)sym;
            err = lock_func(mapper_this, handle, (uint64_t)usage, bounds, vaddr);
        }
        
        return err;
    }

    // Android 11 removed GraphicBufferMapper::unlock(native_handle const*). We forward it to the modern unlock.
    __attribute__((visibility("default")))
    int _ZN7android19GraphicBufferMapper6unlockEPK13native_handle(
            void* mapper_this,
            const android::native_handle* handle) {
        
        static void* libui = []() { void* h = dlopen("libui-v32.so", RTLD_NOW); return h ? h : dlopen("libui.so", RTLD_NOW); }();
        // Modern 2-argument unlock: GraphicBufferMapper::unlock(native_handle const*, unique_fd*)
        static void* sym = libui ? dlsym(libui, "_ZN7android19GraphicBufferMapper6unlockEPK13native_handlePNS_4base14unique_fd_implINS4_13DefaultCloserEEE") : nullptr;
        
        if (sym) {
            typedef int (*UnlockFunc)(void*, const android::native_handle*, int*);
            UnlockFunc unlock_func = (UnlockFunc)sym;
            int fenceFd = -1;
            int err = unlock_func(mapper_this, handle, &fenceFd);
            if (fenceFd >= 0) {
                struct pollfd pfd = {
                    .fd = fenceFd,
                    .events = POLLIN,
                    .revents = 0
                };
                poll(&pfd, 1, -1);
                close(fenceFd);
            }
            return err;
        }
        
        return -1;
    }
}
