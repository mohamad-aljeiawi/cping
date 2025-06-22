package com.cping.jo.utils

object Native {

    fun startNetworkingNative() {
        nativeStartNetworking()

    }


    private external fun nativeStartNetworking()

}