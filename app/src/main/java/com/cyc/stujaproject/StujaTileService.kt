package com.cyc.stujaproject

import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Quick Settings Tile entry point.
 * Tap tile -> cek proses target lagi jalan -> jalanin native scan/edit -> toast hasil.
 */
class StujaTileService : TileService() {

    // Sesuaikan sama daftar package MLBB di loader KIWOLASU (ALLOWED_PACKAGES)
    private val ALLOWED_PACKAGES = listOf(
        "com.hhgame.mlbbvn",
        "com.mobiin.gp",
        "com.mobile.legends",
        "com.mobile.legends.usa",
        "com.sunshine.freeform"
    )

    private val scope = CoroutineScope(Dispatchers.Main)

    override fun onClick() {
        super.onClick()

        scope.launch {
            val targetPkg = withContext(Dispatchers.IO) {
                NativeBridge.findRunningPackage(ALLOWED_PACKAGES)
            }

            if (targetPkg == null) {
                Toast.makeText(applicationContext, "MLBB gak lagi jalan!", Toast.LENGTH_SHORT).show()
                return@launch
            }

            val result = withContext(Dispatchers.IO) {
                NativeBridge.doScan(targetPkg)
            }

            if (result) {
                Toast.makeText(applicationContext, "Berhasil", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(applicationContext, "Gagal — cek root/target", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onStartListening() {
        super.onStartListening()
        qsTile?.state = Tile.STATE_INACTIVE
        qsTile?.updateTile()
    }
}
