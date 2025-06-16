package com.cping.jo.service

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.graphics.PixelFormat
import android.os.Build
import android.os.IBinder
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.drag
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.width
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.ComposeView
import androidx.compose.ui.unit.dp
import androidx.core.app.NotificationCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.LifecycleRegistry
import androidx.lifecycle.setViewTreeLifecycleOwner
import androidx.savedstate.SavedStateRegistry
import androidx.savedstate.SavedStateRegistryController
import androidx.savedstate.SavedStateRegistryOwner
import androidx.savedstate.setViewTreeSavedStateRegistryOwner
import com.cping.jo.R
import com.cping.jo.screens.menu.MenuScreen
import com.cping.jo.ui.theme.CpingTheme
import com.cping.jo.utils.SharedPrefManager
import dagger.hilt.android.AndroidEntryPoint
import jakarta.inject.Inject
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel

@AndroidEntryPoint
class OverlayMenu : Service(), LifecycleOwner, SavedStateRegistryOwner {

    private lateinit var windowManager: WindowManager
    private var overlayMenu: View? = null
    private var gestureMenu: View? = null
    private var overlayMenuParams: WindowManager.LayoutParams? = null
    private var gestureMenuParams: WindowManager.LayoutParams? = null
    private var isOverlayMenuVisible = false
    private var isGestureMenuVisible = false


    private val _lifecycleRegistry = LifecycleRegistry(this)
    private val _savedStateRegistryController: SavedStateRegistryController =
        SavedStateRegistryController.create(this)
    override val savedStateRegistry: SavedStateRegistry =
        _savedStateRegistryController.savedStateRegistry
    override val lifecycle: Lifecycle = _lifecycleRegistry

    private val job = SupervisorJob()
    private val serviceScope = CoroutineScope(Dispatchers.IO + job)

    @Inject
    lateinit var sharedPrefManager: SharedPrefManager


    private fun createOverlayMenuNotification(): Notification {
        val channelId = "overlay_menu_cping"
        val channelName = "CPING Menu Overlay"

        val channel = NotificationChannel(
            channelId,
            channelName,
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = "Control overlay service is running"
            setShowBadge(false)
            setSound(null, null)
        }

        val notificationManager = getSystemService(NotificationManager::class.java)
        notificationManager.createNotificationChannel(channel)

        return NotificationCompat.Builder(this, channelId)
            .setSmallIcon(R.drawable.ic_logo)
            .setContentTitle("CPING Menu Active")
            .setContentText("The in-game control menu is running.")
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setSilent(true)
            .build()
    }


    override fun onCreate() {
        super.onCreate()
        _savedStateRegistryController.performAttach()
        _savedStateRegistryController.performRestore(null)
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_CREATE)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE)
            startForeground(
                1001,
                createOverlayMenuNotification(),
                ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC
            )
        else
            startForeground(1, createOverlayMenuNotification())
    }

    @SuppressLint("RtlHardcoded")
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START)
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_RESUME)

        windowManager = getSystemService(WINDOW_SERVICE) as WindowManager

        overlayMenuParams = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.CENTER
            layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }

        gestureMenuParams = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                    WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS or
                    WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.LEFT or Gravity.CENTER_VERTICAL
            layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }



        overlayMenu = ComposeView(this).apply {
            setViewTreeLifecycleOwner(this@OverlayMenu)
            setViewTreeSavedStateRegistryOwner(this@OverlayMenu)
            setContent {
                CpingTheme {
                    MenuScreen(
                        sharedPrefManager = sharedPrefManager,
                        onHideMenu = {
                            if (!isGestureMenuVisible) {
                                windowManager.addView(gestureMenu, gestureMenuParams)
                                windowManager.removeView(overlayMenu)
                                isOverlayMenuVisible = false
                                isGestureMenuVisible = true
                            }
                        }
                    )
                }
            }
        }

        gestureMenu = ComposeView(this).apply {
            setViewTreeLifecycleOwner(this@OverlayMenu)
            setViewTreeSavedStateRegistryOwner(this@OverlayMenu)
            setContent {
                Box(
                    modifier = Modifier
                        .height(20.dp * 3)
                        .width(30.dp)
                        .pointerInput(Unit) {
                            awaitPointerEventScope {
                                while (true) {
                                    val down = awaitFirstDown(requireUnconsumed = false)
                                    val pointerId = down.id
                                    val startX = down.position.x
                                    var dragTriggered = false

                                    drag(pointerId) { change ->
                                        val dragX = change.position.x
                                        val deltaX = dragX - startX

                                        if (!dragTriggered && deltaX > 60f) {
                                            dragTriggered = true

                                            if (!isOverlayMenuVisible) {
                                                windowManager.addView(
                                                    overlayMenu,
                                                    overlayMenuParams
                                                )
                                                windowManager.removeView(gestureMenu)
                                                isOverlayMenuVisible = true
                                                isGestureMenuVisible = false
                                            }
                                        }
                                    }
                                }
                            }
                        }
                )
            }
        }
        windowManager.addView(gestureMenu, gestureMenuParams)

        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        if (isOverlayMenuVisible) windowManager.removeView(overlayMenu)
        if (isGestureMenuVisible) windowManager.removeView(gestureMenu)
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_PAUSE)
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_STOP)
        _lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY)
        serviceScope.cancel()

    }

    override fun onBind(intent: Intent?): IBinder? = null

    companion object {
        internal fun showOverlay(
            context: Context,
//            dynamicPath: String,
//            packageName: String
        ) {
            val intent = Intent(context, OverlayMenu::class.java)
//            intent.putExtra(Constants.INTENT_VALUE_DYNAMIC_PATH, dynamicPath)
//            intent.putExtra(Constants.INTENT_VALUE_PACKAGE_NAME, packageName)
            context.startForegroundService(intent)

        }


        internal fun hideOverlay(context: Context) {
            val intent = Intent(context, OverlayMenu::class.java)
            context.stopService(intent)
        }
    }
}