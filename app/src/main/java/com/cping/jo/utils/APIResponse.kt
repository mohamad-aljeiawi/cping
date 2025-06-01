package com.cping.jo.utils

sealed class APIResponse<T> {
    data class Success<T>(val data: T) : APIResponse<T>()
    data class Error<T>(val error: Throwable) : APIResponse<T>()
    class Loading<T> : APIResponse<T>()
    class Idle<T> : APIResponse<T>()
}

