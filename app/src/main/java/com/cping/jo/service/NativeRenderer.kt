package com.cping.jo.service

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.OrientationEventListener
import javax.microedition.khronos.opengles.GL10
import javax.microedition.khronos.egl.EGLConfig

class NativeRenderer(
    private val context: Context
) : GLSurfaceView.Renderer {

    private var orientationListener: OrientationEventListener? = null

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        nativeOnSurfaceCreated()
        startOrientationTracking()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeOnSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeOnDrawFrame()
    }


    private fun startOrientationTracking() {
        orientationListener = object : OrientationEventListener(context) {
            override fun onOrientationChanged(angle: Int) {
                if (angle == ORIENTATION_UNKNOWN) return

                val orientation = when {
                    angle in 45..134 -> 90
                    angle in 135..224 -> 180
                    angle in 225..314 -> 270
                    else -> 0
                }

                nativeOnOrientationChanged(orientation)
            }
        }
        orientationListener?.enable()
    }

    private fun stopOrientationTracking() {
        orientationListener?.disable()
        orientationListener = null
    }


    fun release() {
        stopOrientationTracking()
        nativeSurfaceStop()
    }

    companion object {
        @JvmStatic
        external fun nativeOnSurfaceCreated()

        @JvmStatic
        external fun nativeOnDrawFrame()

        @JvmStatic
        external fun nativeOnSurfaceChanged(width: Int, height: Int)

        @JvmStatic
        external fun nativeOnOrientationChanged(orientation: Int)

        @JvmStatic
        external fun nativeSurfaceStop()
    }
}