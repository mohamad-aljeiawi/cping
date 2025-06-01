package com.cping.jo

import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.tooling.preview.Preview
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import com.cping.jo.navigation.graphs.RootGraph
import com.cping.jo.ui.theme.CpingTheme
import com.cping.jo.utils.Constants
import com.cping.jo.utils.Utils
import com.russhwolf.settings.BuildConfig
import com.topjohnwu.superuser.Shell
import dagger.hilt.android.AndroidEntryPoint


@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    companion object {
        init {
            System.loadLibrary("cping_socket")

            Shell.enableVerboseLogging = BuildConfig.DEBUG
            Shell.setDefaultBuilder(
                Shell.Builder.create()
                    .setFlags(Shell.FLAG_MOUNT_MASTER)
                    .setTimeout(30)
            )
        }
    }

    var splashState by mutableStateOf(true)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        actionBar?.hide()
        val splashScreen = installSplashScreen()
        splashScreen.setKeepOnScreenCondition {
            splashState
        }

        Shell.getShell {
            if (it.isRoot) {
                Utils.processBuilderShell("settings put global block_untrusted_touches 1")
                Utils.processBuilderShell("pm grant ${Constants.PACKAGE_NAME} android.permission.WRITE_EXTERNAL_STORAGE")
                Utils.processBuilderShell("pm grant ${Constants.PACKAGE_NAME} android.permission.READ_EXTERNAL_STORAGE")
                Utils.processBuilderShell("pm grant ${Constants.PACKAGE_NAME} android.permission.READ_PHONE_STATE")

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
                    !Environment.isExternalStorageManager()
                ) {
                    Utils.processBuilderShell("am start -a android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION -d package:${Constants.PACKAGE_NAME}")
                }

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q &&
                    !Settings.canDrawOverlays(this)
                ) {
                    Utils.processBuilderShell("am start -a android.settings.action.MANAGE_OVERLAY_PERMISSION -d package:${Constants.PACKAGE_NAME}")
                }

                setContent {
                    CpingTheme {
                        RootGraph(
                            onSplashFinished = {
                                splashState = false
                            }
                        )
                    }
                }

            } else {
                setContent {
                    CpingTheme {
                        RootGraph(
                            onSplashFinished = {
                                splashState = false
                            }
                        )
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
//        OverlayGame.hideOverlay(this)
//        OverlayMenu.hideOverlay(this)
//        Utils.processBuilderShell("kill $(pidof ${Constants.CPING_MEMORY})")
    }
}


@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    CpingTheme {
        RootGraph()
    }
}