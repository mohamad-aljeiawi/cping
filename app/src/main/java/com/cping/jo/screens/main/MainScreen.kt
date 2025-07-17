package com.cping.jo.screens.main

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import coil3.compose.SubcomposeAsyncImage
import coil3.request.ImageRequest
import coil3.request.crossfade
import com.cping.jo.model.GameObject
import com.cping.jo.model.UserProfileUi
import com.cping.jo.ui.theme.CpingTheme
import com.cping.jo.utils.APIResponse
import com.cping.jo.R
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import java.time.temporal.ChronoUnit

@Composable
fun MainScreen(
    rootNavController: NavHostController, viewModel: MainViewModel = hiltViewModel()
) {
    val userProfileState by viewModel.userProfileState.collectAsState()
    val gamesState by viewModel.gamesState.collectAsState()

    val userProfile = (userProfileState as? APIResponse.Success<UserProfileUi>)?.data
    val games = (gamesState as? APIResponse.Success<List<GameObject>>)?.data ?: emptyList()

    LayoutMain(
        user = userProfile,
        games = games,
        loading = userProfileState is APIResponse.Loading,
        error = (userProfileState as? APIResponse.Error<UserProfileUi>)?.error?.localizedMessage?.substringBefore(
            "\n"
        )
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LayoutMain(
    user: UserProfileUi? = null,
    games: List<GameObject> = emptyList(),
    loading: Boolean = false,
    error: String? = null,
) {
    var showError by remember { mutableStateOf(false) }
    LaunchedEffect(error) { showError = error != null }

    Scaffold(
        modifier = Modifier.fillMaxSize(), containerColor = MaterialTheme.colorScheme.background
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(
                    start = 24.dp,
                    end = 24.dp,
                    top = paddingValues.calculateTopPadding(),
                    bottom = 48.dp
                ), horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Spacer(modifier = Modifier.height(64.dp))
            HeroSection(user = user)
            Spacer(modifier = Modifier.height(16.dp))
            GameList(games = games)
        }
    }
}

@Composable
fun HeroSection(user: UserProfileUi? = null) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = MaterialTheme.shapes.medium,
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(
                imageVector = Icons.Default.Person,
                contentDescription = "User Icon",
                tint = MaterialTheme.colorScheme.primary,
                modifier = Modifier.size(40.dp)
            )
            Spacer(modifier = Modifier.width(12.dp))

            Column(
                modifier = Modifier.weight(1f)
            ) {
                Text(
                    text = user?.fullName ?: "اسم المستخدم",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurface
                )

                Text(
                    text = "@${user?.username ?: "username"}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }

            Text(
                text = if (user?.isPremium == true) stringResource(id = R.string.main_user_premium) else stringResource(
                    id = R.string.main_user_free
                ),
                color = if (user?.isPremium == true) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.secondary,
                style = MaterialTheme.typography.bodyMedium,
                fontWeight = FontWeight.SemiBold
            )
        }
    }
}

@Composable
fun GameList(games: List<GameObject>) {
    Column(
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            text = stringResource(id = R.string.main_games_available),
            style = MaterialTheme.typography.titleLarge,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.onSurface,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn {
            items(games) { game ->
                GameListItem(game = game)
                Spacer(modifier = Modifier.height(16.dp))
            }
        }
    }
}


@Composable
fun GameListItem(game: GameObject) {
    val formatter = remember { DateTimeFormatter.ISO_DATE_TIME }
    val createdAtDate = remember(game.createdAt) { LocalDateTime.parse(game.createdAt, formatter) }
    val now = remember { LocalDateTime.now() }
    val isNew = ChronoUnit.DAYS.between(createdAtDate, now) <= 2

    val viewModel: MainViewModel = hiltViewModel()
    val context = LocalContext.current
    val isRunning = viewModel.runningGames[game.slug] == true
    val isLoading = viewModel.loadingGames[game.slug] == true

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = MaterialTheme.shapes.medium,
        border = BorderStroke(width = 1.dp, color = MaterialTheme.colorScheme.primary)
    ) {
        Box(
            modifier = Modifier
                .clip(MaterialTheme.shapes.large)
                .fillMaxWidth()
                .height(200.dp)
        ) {
            SubcomposeAsyncImage(
                modifier = Modifier.fillMaxSize(),
                model = ImageRequest.Builder(LocalContext.current).data(game.imageUrl)
                    .crossfade(true).build(),
                contentDescription = game.name,
                contentScale = ContentScale.Crop,
                loading = {
                    Box(
                        modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center
                    ) {
                        CircularProgressIndicator()
                    }
                })

            if (isNew) {
                Box(
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .padding(8.dp)
                        .background(
                            MaterialTheme.colorScheme.errorContainer,
                            shape = MaterialTheme.shapes.small
                        )
                        .padding(horizontal = 8.dp, vertical = 4.dp)
                ) {
                    Text(
                        text = stringResource(id = R.string.main_new_game),
                        color = MaterialTheme.colorScheme.onErrorContainer,
                        style = MaterialTheme.typography.labelSmall,
                        fontWeight = FontWeight.Bold
                    )
                }
            }

            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            colors = listOf(
                                Color.Transparent, Color(0xCC000000)
                            ), startY = 100f, endY = Float.POSITIVE_INFINITY
                        )
                    )
            )

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
                    .align(Alignment.TopEnd),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.End
            ) {
                Button(
                    onClick = {
                        viewModel.toggleGameExecution(context, game)
                    },
                    enabled = !isLoading,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (isRunning) Color.Red else MaterialTheme.colorScheme.primary
                    ),
                    contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
                    shape = MaterialTheme.shapes.small
                ) {
                    if (isLoading) {
                        CircularProgressIndicator(
                            color = Color.White,
                            modifier = Modifier.size(20.dp),
                            strokeWidth = 2.dp
                        )
                    } else {
                        Text(
                            text = if (isRunning) "Stop" else stringResource(id = R.string.main_stsrt),
                            style = MaterialTheme.typography.labelLarge.copy(fontWeight = FontWeight.Bold),
                            color = MaterialTheme.colorScheme.onPrimary
                        )
                    }
                }
            }

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
                    .align(Alignment.BottomStart),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(
                    modifier = Modifier.weight(1f)
                ) {
                    Text(
                        text = game.name, style = MaterialTheme.typography.bodyLarge.copy(
                            color = Color.White, fontWeight = FontWeight.Bold, shadow = Shadow(
                                color = Color.Black, blurRadius = 4f
                            )
                        )
                    )
                    Text(
                        text = game.description, style = MaterialTheme.typography.bodySmall.copy(
                            color = Color.LightGray
                        ), maxLines = 4, overflow = TextOverflow.Ellipsis
                    )
                }
            }
        }
    }
}


@Preview(
    showBackground = true, name = "Main Screen - AR", locale = "ar"
)
@Composable
private fun MainScreenPreview() {
    CpingTheme(darkTheme = true) {
        LayoutMain()
    }
}

@Preview(
    showBackground = true, name = "Main Screen - Dark", locale = "en"
)
@Composable
private fun MainScreenDarkPreview() {
    CpingTheme(darkTheme = true) {
        LayoutMain()
    }
}