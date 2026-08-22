#include <jni.h>
#include <string>
#include "MemoryScanner.h"

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_cyc_stujaproject_NativeBridge_nativeIsPackageRunning(
        JNIEnv *env, jobject /* this */, jstring packageName) {
    const char *pkg = env->GetStringUTFChars(packageName, nullptr);
    int pid = MemoryScanner::findPid(pkg);
    env->ReleaseStringUTFChars(packageName, pkg);
    return pid > 0;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_cyc_stujaproject_NativeBridge_doScan(
        JNIEnv *env, jobject /* this */, jstring packageName) {

    const char *pkgChars = env->GetStringUTFChars(packageName, nullptr);
    std::string pkg(pkgChars);
    env->ReleaseStringUTFChars(packageName, pkgChars);

    int pid = MemoryScanner::findPid(pkg);
    if (pid <= 0) return JNI_FALSE;

    MemoryScanner scanner;
    if (!scanner.attach(pid)) return JNI_FALSE;

    bool success = false;

    // --- Port dari TES() ---
    // gg.searchNumber("2320;133;250", DWORD)
    auto results = scanner.searchSequence({2320, 133, 250});
    // gg.searchNumber("133;250", DWORD)  -> refine, geser 1 elemen (skip anchor 2320)
    results = scanner.refineSequence(results, {133, 250});

    if (!results.empty()) {
        // gg.editAll("11111", DWORD)  -- tulis ke elemen pertama hasil refine (yang tadinya "133")
        scanner.editAllAt(results, 0, 11111);

        // clearResults() -> search baru: "2320;11111"
        auto results2 = scanner.searchSequence({2320, 11111});
        // search("240" doang di versi asli -> refine ke {2320} doang
        results2 = scanner.refineSequence(results2, {2320});

        if (!results2.empty()) {
            scanner.editAllAt(results2, 0, 2500);
            success = true;
        }
    }

    scanner.detach();
    return success ? JNI_TRUE : JNI_FALSE;
}
