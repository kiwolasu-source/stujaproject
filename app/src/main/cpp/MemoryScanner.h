#pragma once
#include <string>
#include <vector>
#include <cstdint>

class MemoryScanner {
public:
    // Cari PID dari nama package (baca /proc/[pid]/cmdline). -1 kalau gak ketemu.
    static int findPid(const std::string &packageName);

    // Attach ptrace ke pid target. Butuh root (CAP_SYS_PTRACE) buat cross-process.
    bool attach(int pid);
    void detach();

    // Search DWORD (4 byte) berurutan sesuai pattern, dalam region memory writable/anon.
    // pattern: list nilai int32 yang harus muncul berurutan (offset 4 byte tiap elemen).
    // Return: base address tiap match (address elemen pertama).
    std::vector<uintptr_t> searchSequence(const std::vector<int32_t> &pattern);

    // Refine: dari existing base addresses, cek ulang apakah sequence baru masih cocok
    // pada offset yang sama (dipakai buat search kedua tanpa clear).
    std::vector<uintptr_t> refineSequence(const std::vector<uintptr_t> &bases,
                                           const std::vector<int32_t> &pattern);

    // Tulis DWORD ke semua base address di offset tertentu (elemen ke berapa dalam pattern asli).
    bool editAllAt(const std::vector<uintptr_t> &bases, size_t elementOffset, int32_t value);

private:
    int pid_ = -1;
    bool attached_ = false;

    bool readMem(uintptr_t addr, void *buf, size_t len);
    bool writeMem(uintptr_t addr, const void *buf, size_t len);
    std::vector<std::pair<uintptr_t, uintptr_t>> readableWritableRegions();
};
