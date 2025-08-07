package com.cping.jo

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.tooling.preview.Preview
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import com.cping.jo.navigation.graphs.RootGraph
import com.cping.jo.ui.theme.CpingTheme
import com.russhwolf.settings.BuildConfig
import com.topjohnwu.superuser.Shell
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    companion object {
        init {
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
        splashScreen.setKeepOnScreenCondition { splashState }

        Shell.getShell { shell ->
            shell.newJob().add("id").submit()
            if (shell.isRoot) {
                shell.newJob().add("settings put global block_untrusted_touches 1").submit()
                setContent {
                    CpingTheme {
                        RootGraph {
                            splashState = false
                        }
                    }
                }
            } else {
                Toast.makeText(this, "Root access not granted", Toast.LENGTH_LONG).show()
                setContent {
                    CpingTheme {
                        RootGraph {
                            splashState = false
                        }
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
    }
}


@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    CpingTheme { RootGraph() }
}