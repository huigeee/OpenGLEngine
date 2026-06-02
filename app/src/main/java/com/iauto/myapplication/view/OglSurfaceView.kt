package com.iauto.myapplication.view

import android.content.Context
import android.view.MotionEvent
import android.util.AttributeSet
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * OglSurfaceView
 * - 承载 OpenGL 渲染的 SurfaceView
 * - 处理触摸事件，委托给 OglModelManager
 * - 连接 OglRendererService（通过 SurfaceHolder.Callback）
 */
class OglSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr) {

    private val holderCallback = object : SurfaceHolder.Callback {
        override fun surfaceCreated(holder: SurfaceHolder) {
            Log.i(TAG, "surfaceCreated")
            listener?.onSurfaceCreated(holder.surface)
        }

        override fun surfaceChanged(
            holder: SurfaceHolder,
            format: Int,
            width: Int,
            height: Int
        ) {
            Log.i(TAG, "surfaceChanged: ${width}x${height}")
            listener?.onSurfaceChanged(width, height)
        }

        override fun surfaceDestroyed(holder: SurfaceHolder) {
            Log.i(TAG, "surfaceDestroyed")
            listener?.onSurfaceDestroyed()
        }
    }

    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var isDragging = false

    interface Listener {
        fun onSurfaceCreated(surface: android.view.Surface)
        fun onSurfaceChanged(width: Int, height: Int)
        fun onSurfaceDestroyed()
        fun onRotation(deltaX: Float, deltaY: Float)
    }

    var listener: Listener? = null

    companion object {
        private const val TAG = "OglSurfaceView"
    }

    init {
        holder.addCallback(holderCallback)
        // 确保 SurfaceView 能够接收触摸事件
        isClickable = true
        isFocusable = false
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (event == null) return false

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                lastTouchX = event.x
                lastTouchY = event.y
                isDragging = true
            }
            MotionEvent.ACTION_MOVE -> {
                if (isDragging) {
                    val deltaX = event.x - lastTouchX
                    val deltaY = event.y - lastTouchY
                    listener?.onRotation(deltaX, deltaY)
                    lastTouchX = event.x
                    lastTouchY = event.y
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                isDragging = false
            }
        }
        return true
    }
    
    override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
        return super.dispatchTouchEvent(ev)
    }

    // 暴露给 Manager 调用的方法
    fun setRotation(deltaX: Float, deltaY: Float) {
        listener?.onRotation(deltaX, deltaY)
    }

    // 扩展：支持外部设置触摸监听
    fun setOnRotationListener(listener: ((Float, Float) -> Unit)?) {
        object : Listener {
            override fun onSurfaceCreated(surface: android.view.Surface) {}
            override fun onSurfaceChanged(width: Int, height: Int) {}
            override fun onSurfaceDestroyed() {}
            override fun onRotation(deltaX: Float, deltaY: Float) {
                listener?.invoke(deltaX, deltaY)
            }
        }
    }

    fun release() {
        holder.removeCallback(holderCallback)
        listener = null
    }
}
