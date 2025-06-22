package com.cping.jo.utils

object NativeBridge {

    fun dispatchMenuEvent(key: String, isChecked: Boolean, sliderValue: Float) {
        onMenuEvent(key = key, isChecked = isChecked, sliderValue = sliderValue)
    }

    external fun onMenuEvent(key: String, isChecked: Boolean, sliderValue: Float)

}