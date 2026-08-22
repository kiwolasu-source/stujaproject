package com.cyc.stujaproject

object NativeBridge {
    init {
        System.loadLibrary("stuja")
    }

    /** Cari package mana dari daftar allowed yang lagi jalan. Null kalau gak ada. */
    fun findRunningPackage(allowed: List<String>): String? {
        for (pkg in allowed) {
            if (nativeIsPackageRunning(pkg)) return pkg
        }
        return null
    }

    /** Jalanin pipeline scan/edit (port dari TES() Lua) ke package target. */
    external fun doScan(packageName: String): Boolean

    private external fun nativeIsPackageRunning(packageName: String): Boolean
}
