package com.cping.jo.screens.main

import android.content.Context
import androidx.compose.runtime.mutableStateMapOf
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.cping.jo.model.GameObject
import com.cping.jo.model.UserProfileUi
import com.cping.jo.repository.AppRepository
import com.cping.jo.utils.APIResponse
import com.cping.jo.utils.Constants
import com.cping.jo.utils.SharedPrefManager
import com.cping.jo.utils.Utils
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import jakarta.inject.Inject
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

@HiltViewModel
class MainViewModel @Inject constructor(
    private val appRepository: AppRepository,
    private val sharedPrefManager: SharedPrefManager,
    @ApplicationContext private val context: Context,
) : ViewModel() {
    private val _userProfileState =
        MutableStateFlow<APIResponse<UserProfileUi>>(APIResponse.Loading())
    val userProfileState: StateFlow<APIResponse<UserProfileUi>> = _userProfileState

    private val _gamesState = MutableStateFlow<APIResponse<List<GameObject>>>(APIResponse.Loading())
    val gamesState: StateFlow<APIResponse<List<GameObject>>> = _gamesState

    private val _runningGames = mutableStateMapOf<String, Boolean>()
    val runningGames: Map<String, Boolean> get() = _runningGames

    private val _loadingGames = mutableStateMapOf<String, Boolean>()
    val loadingGames: Map<String, Boolean> get() = _loadingGames


    init {
        viewModelScope.launch {
            val personId = sharedPrefManager.getData(Constants.PREF_PERSON_ID)
            _userProfileState.value = appRepository.getCurrentUserProfile(personId.toString())
        }

        viewModelScope.launch {
            _gamesState.value = appRepository.getGames()
        }

        val runningSet = loadRunningSet(sharedPrefManager)
        runningSet.forEach { slug ->
            _runningGames[slug] = true
        }
    }


    fun saveRunningSet(sharedPrefManager: SharedPrefManager, runningSet: Set<String>) {
        val value = runningSet.joinToString(",")
        sharedPrefManager.putData(Constants.PREF_RUNNING_KEY, value)
    }

    fun loadRunningSet(sharedPrefManager: SharedPrefManager): Set<String> {
        return sharedPrefManager.getData(Constants.PREF_RUNNING_KEY)
            ?.split(",")
            ?.filter { it.isNotBlank() }
            ?.toSet() ?: emptySet()
    }

    fun toggleGameExecution(context: Context, game: GameObject) {
        val slug = game.slug
        val fileName = game.downloadUrl.substringAfterLast("/")
        val isRunning = _runningGames[slug] == true

        if (isRunning) {
            _runningGames[slug] = false
            Utils.processBuilderShell("kill -9 \$(pidof $fileName)")
            saveRunningSet(sharedPrefManager, _runningGames.filterValues { it }.keys)
        } else {
            _loadingGames[slug] = true
            Utils.downloadFileIfNotExists(context, game.downloadUrl, fileName) { success ->
                if (success) {
                    Utils.runFileInTmp(fileName)
                    _runningGames[slug] = true
                }
                _loadingGames[slug] = false
                saveRunningSet(sharedPrefManager, _runningGames.filterValues { it }.keys)
            }
        }
    }
}