package ca.polymtl.inf3990_01.client.controller

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import ca.polymtl.inf3990_01.client.controller.event.*
import ca.polymtl.inf3990_01.client.controller.rest.RestRequestService
import ca.polymtl.inf3990_01.client.controller.rest.SecureRestRequestService
import ca.polymtl.inf3990_01.client.model.User
import ca.polymtl.inf3990_01.client.presentation.Presenter
import kotlinx.coroutines.experimental.Job
import kotlinx.coroutines.experimental.async
import kotlinx.coroutines.experimental.launch
import java.util.concurrent.ScheduledFuture
import java.util.concurrent.ScheduledThreadPoolExecutor
import java.util.concurrent.TimeUnit

class AppController(
        private val eventMgr: EventManager,
        private val restService: RestRequestService,
        private val secureRestService: SecureRestRequestService,
        private val presenter: Presenter,
        private val preferences: SharedPreferences,
        private val localSongController: LocalSongController,
        private val appCtx: Context
) {
    companion object {
        const val QUEUE_PERIOD_KEY = "queue_period"
        const val QUEUE_PERIOD_DEFAULT = 4000L
    }

    private val executor = ScheduledThreadPoolExecutor(1)

    private var reloadQueueJob: Job? = null
    private var task: ScheduledFuture<*>? = null
    private var reloadBlackListJob: Job? = null

    private val prefChangeListener = SharedPreferences.OnSharedPreferenceChangeListener {sharedPreferences, key ->
        if (key == QUEUE_PERIOD_KEY) {
            task?.cancel(true)
            task = scheduleQueueTask(sharedPreferences)
        }
    }

    init {
        eventMgr.addEventListener(this::onAppInit)
        eventMgr.addEventListener(this::onAppStart)
        eventMgr.addEventListener(this::onAppStop)
        eventMgr.addEventListener(this::reloadQueue)
        eventMgr.addEventListener(this::onSendSong)
        eventMgr.addEventListener(this::onReloadLocalSong)
        eventMgr.addEventListener(this::reloadBlackListUser)
    }

    @Suppress("UNUSED_PARAMETER")
    private fun onAppInit(event: AppInitEvent) {
        eventMgr.removeEventListener(this::onAppInit) // Run it just once
        // Setup the preference listener
        preferences.registerOnSharedPreferenceChangeListener(prefChangeListener)
    }

    /**
     * When the app resumes from inactivity (when starting or coming back from being paused), we start the reload loop.
     */
    private fun onAppStart(event: AppResumeEvent) {
        // Make sure that the previous task was stopped
        task?.cancel(true)
        // Start the updating loop
        task = scheduleQueueTask(preferences)
    }

    /**
     * When the app is no longer visible (when no activities are being shown, when paused), we stop the reload loop.
     */
    @Suppress("UNUSED_PARAMETER")
    private fun onAppStop(event: AppStopEvent) {
        task?.cancel(true)
    }

    @Suppress("UNUSED_PARAMETER")
    private fun reloadQueue(event: RequestQueueReloadEvent) {
        Log.d("AppController", "Reloading the song's queue")
        if (reloadQueueJob?.isCompleted != false) {
            val jobTmp = reloadQueueJob
            reloadQueueJob = async {
                jobTmp?.join()
                val list = restService.getSongList()
                // now, we update the model
                presenter.setQueue(list)
            }
        }
    }


    @Suppress("UNUSED_PARAMETER")
    private fun reloadBlackListUser(event: RequestBlackListReloadEvent) {
        Log.d("AppController", "Reloading the black list of users")
        if (reloadBlackListJob?.isCompleted != false) {
            val jobTmp = reloadBlackListJob
            reloadBlackListJob = async {
                jobTmp?.join()
                val list = secureRestService.getBlackList()
                // now, we update the model
                presenter.setBlackListOfUsers(list)
            }
        }
    }
    private fun scheduleQueueTask(prefs: SharedPreferences): ScheduledFuture<*> {
        return executor.scheduleAtFixedRate({
            eventMgr.dispatchEvent(RequestQueueReloadEvent())
        }, 0, prefs.getString(QUEUE_PERIOD_KEY, "$QUEUE_PERIOD_DEFAULT")?.toLong() ?: QUEUE_PERIOD_DEFAULT, TimeUnit.MILLISECONDS)
    }

    private fun onSendSong(event: SendSongEvent) {
        launch {
            restService.sendSong(event.song)
        }
    }

    private fun onReloadLocalSong(event: LocalSongLoadEvent) {
        localSongController.reloadLocalSong()
    }
}