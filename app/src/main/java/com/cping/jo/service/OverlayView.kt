package com.cping.jo.service

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
import android.view.WindowManager
import androidx.core.app.NotificationCompat
import com.cping.jo.R

class OverlayView : Service() {

    private lateinit var windowManager: WindowManager
    private lateinit var glOverlayView: GLOverlayView

    private fun createOverlayViewNotification(): Notification {
        val channelId = "overlay_view_cping"
        val channelName = "CPING Visual Overlay"

        val channel = NotificationChannel(
            channelId,
            channelName,
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = "Visual overlay service is running"
            setShowBadge(false)
            setSound(null, null)
        }

        val notificationManager = getSystemService(NotificationManager::class.java)
        notificationManager.createNotificationChannel(channel)

        return NotificationCompat.Builder(this, channelId)
            .setSmallIcon(R.drawable.ic_logo)
            .setContentTitle("CPING Visual Overlay")
            .setContentText("The visual overlay is active and running.")
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setSilent(true)
            .build()
    }

    override fun onCreate() {
        super.onCreate()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE)
            startForeground(
                1001,
                createOverlayViewNotification(),
                ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC
            )
        else
            startForeground(1, createOverlayViewNotification())
    }


    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
//        dynamicPath = intent?.getStringExtra(Constants.INTENT_VALUE_DYNAMIC_PATH)
//        packageName = intent?.getStringExtra(Constants.INTENT_VALUE_PACKAGE_NAME)
        val context = applicationContext

        glOverlayView = GLOverlayView(this)
        windowManager = getSystemService(WINDOW_SERVICE) as WindowManager

        val layoutParams = WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            0, 0,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                    WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
                    WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or
                    WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            PixelFormat.TRANSLUCENT
        ).apply {
            layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }

        windowManager.addView(glOverlayView, layoutParams)

        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        glOverlayView.release()
        windowManager.removeView(glOverlayView)
    }

    override fun onBind(intent: Intent?): IBinder? = null


    companion object {
        internal fun showOverlay(
            context: Context,
//            dynamicPath: String,
//            packageName: String
        ) {
            val intent = Intent(context, OverlayView::class.java)
//            intent.putExtra(Constants.INTENT_VALUE_DYNAMIC_PATH, dynamicPath)
//            intent.putExtra(Constants.INTENT_VALUE_PACKAGE_NAME, packageName)
            context.startForegroundService(intent)

        }


        internal fun hideOverlay(context: Context) {
            val intent = Intent(context, OverlayView::class.java)
            context.stopService(intent)
        }
    }
}
