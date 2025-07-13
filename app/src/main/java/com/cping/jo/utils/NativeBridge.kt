package com.cping.jo.utils

object NativeBridge {

    fun dispatchMenuEvent(key: Int, isChecked: Boolean, sliderValue: Float) {
        onMenuEvent(key = key, isChecked = isChecked, sliderValue = sliderValue)
    }

    external fun onMenuEvent(key: Int, isChecked: Boolean, sliderValue: Float)

}