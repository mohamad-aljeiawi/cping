package com.cping.jo.service

import android.opengl.GLSurfaceView
import javax.microedition.khronos.opengles.GL10
import javax.microedition.khronos.egl.EGLConfig

class NativeRenderer : GLSurfaceView.Renderer {

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        nativeOnSurfaceCreated()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeOnSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeOnDrawFrame()
    }

    fun release() {
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
        external fun nativeSurfaceStop()
    }
}