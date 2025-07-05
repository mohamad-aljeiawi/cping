package com.cping.jo.service


import android.content.Context
import android.graphics.PixelFormat
import android.opengl.GLSurfaceView
import javax.microedition.khronos.egl.EGL10
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.egl.EGLDisplay

class GLOverlayView(context: Context) : GLSurfaceView(context) {
    private val renderer = NativeRenderer()

    init {
        setZOrderOnTop(true)
        holder.setFormat(PixelFormat.TRANSLUCENT)

        setEGLContextClientVersion(3)

        setEGLConfigChooser(object : EGLConfigChooser {
            override fun chooseConfig(egl: EGL10, display: EGLDisplay): EGLConfig {
                val attribs = intArrayOf(
                    EGL10.EGL_RED_SIZE, 8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_ALPHA_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, 16,
                    EGL10.EGL_RENDERABLE_TYPE, 4,
                    EGL10.EGL_NONE
                )
                val num = IntArray(1)
                val cogs = arrayOfNulls<EGLConfig>(1)
                egl.eglChooseConfig(display, attribs, cogs, 1, num)
                return cogs[0]!!
            }
        })

        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    fun release() {
        renderer.release()
    }
}