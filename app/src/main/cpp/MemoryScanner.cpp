#include "MemoryScanner.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <android/log.h>

#define TAG "StujaScanner"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

int MemoryScanner::findPid(const std::string &packageName) {
    DIR *procDir = opendir("/proc");
    if (!procDir) return -1;

    struct dirent *entry;
    while ((entry = readdir(procDir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;

        std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";
        FILE *f = fopen(cmdlinePath.c_str(), "r");
        if (!f) continue;

        char buf[256] = {0};
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);

        if (packageName == buf) {
            closedir(procDir);
            return atoi(entry->d_name);
        }
    }
    closedir(procDir);
    return -1;
}

bool MemoryScanner::attach(int pid) {
    // NOTE v1: pakai ptrace langsung. Ini cuma jalan kalau proses kita
    // (Stuja) punya CAP_SYS_PTRACE / jalan sebagai root. Di device non-root
    // ini bakal gagal (EPERM) -- perlu fallback lain kalau target non-root.
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) != 0) {
        LOGE("ptrace attach gagal pid=%d errno=%d", pid, errno);
        return false;
    }
    int status;
    waitpid(pid, &status, 0);
    pid_ = pid;
    attached_ = true;
    return true;
}

void MemoryScanner::detach() {
    if (attached_) {
        ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
        attached_ = false;
    }
}

bool MemoryScanner::readMem(uintptr_t addr, void *buf, size_t len) {
    std::string memPath = "/proc/" + std::to_string(pid_) + "/mem";
    int fd = open(memPath.c_str(), O_RDONLY);
    if (fd < 0) return false;
    bool ok = pread(fd, buf, len, addr) == (ssize_t) len;
    close(fd);
    return ok;
}

bool MemoryScanner::writeMem(uintptr_t addr, const void *buf, size_t len) {
    std::string memPath = "/proc/" + std::to_string(pid_) + "/mem";
    int fd = open(memPath.c_str(), O_WRONLY);
    if (fd < 0) return false;
    bool ok = pwrite(fd, buf, len, addr) == (ssize_t) len;
    close(fd);
    return ok;
}

std::vector<std::pair<uintptr_t, uintptr_t>> MemoryScanner::readableWritableRegions() {
    std::vector<std::pair<uintptr_t, uintptr_t>> regions;
    std::string mapsPath = "/proc/" + std::to_string(pid_) + "/maps";
    FILE *f = fopen(mapsPath.c_str(), "r");
    if (!f) return regions;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        uintptr_t start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) == 3) {
            // rw- region (heap/anon) -- lokasi umum value gameplay (skor, hp, dll)
            if (perms[0] == 'r' && perms[1] == 'w') {
                regions.emplace_back(start, end);
            }
        }
    }
    fclose(f);
    return regions;
}

std::vector<uintptr_t> MemoryScanner::searchSequence(const std::vector<int32_t> &pattern) {
    std::vector<uintptr_t> results;
    if (pattern.empty()) return results;

    size_t spanBytes = pattern.size() * sizeof(int32_t);
    auto regions = readableWritableRegions();

    for (auto &region : regions) {
        uintptr_t start = region.first;
        uintptr_t end = region.second;
        size_t regionSize = end - start;
        if (regionSize > 64 * 1024 * 1024) continue; // skip region raksasa, v1 keep it simple

        std::vector<uint8_t> buf(regionSize);
        if (!readMem(start, buf.data(), regionSize)) continue;

        for (size_t off = 0; off + spanBytes <= regionSize; off += 4) {
            bool match = true;
            for (size_t i = 0; i < pattern.size(); i++) {
                int32_t val;
                memcpy(&val, buf.data() + off + i * 4, 4);
                if (val != pattern[i]) { match = false; break; }
            }
            if (match) results.push_back(start + off);
        }
    }
    LOGI("searchSequence: %zu match", results.size());
    return results;
}

std::vector<uintptr_t> MemoryScanner::refineSequence(const std::vector<uintptr_t> &bases,
                                                       const std::vector<int32_t> &pattern) {
    std::vector<uintptr_t> results;
    for (auto base : bases) {
        bool match = true;
        for (size_t i = 0; i < pattern.size(); i++) {
            int32_t val;
            if (!readMem(base + i * 4, &val, 4) || val != pattern[i]) { match = false; break; }
        }
        if (match) results.push_back(base);
    }
    LOGI("refineSequence: %zu -> %zu match", bases.size(), results.size());
    return results;
}

bool MemoryScanner::editAllAt(const std::vector<uintptr_t> &bases, size_t elementOffset, int32_t value) {
    bool anyOk = false;
    for (auto base : bases) {
        if (writeMem(base + elementOffset * 4, &value, 4)) anyOk = true;
    }
    return anyOk;
}
