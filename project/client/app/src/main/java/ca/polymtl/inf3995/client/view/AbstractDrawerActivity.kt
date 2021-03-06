package ca.polymtl.inf3995.client.view

import android.content.Intent
import android.media.AudioManager
import android.os.Bundle
import android.support.annotation.IdRes
import android.support.annotation.LayoutRes
import android.support.design.widget.NavigationView
import android.support.v4.view.GravityCompat
import android.support.v4.widget.DrawerLayout
import android.support.v7.app.ActionBarDrawerToggle
import android.support.v7.app.AppCompatActivity
import android.support.v7.widget.Toolbar
import android.view.MenuItem
import ca.polymtl.inf3995.client.R
import ca.polymtl.inf3995.client.controller.ActiveActivityTrackingService
import ca.polymtl.inf3995.client.controller.state.AppStateService
import org.koin.android.ext.android.inject
import java.util.*

@Suppress("MemberVisibilityCanBePrivate")
abstract class AbstractDrawerActivity(
    @LayoutRes private val layoutRes: Int,
    @IdRes private val drawerLayoutId: Int
): AppCompatActivity(), NavigationView.OnNavigationItemSelectedListener {

    protected val stateService: AppStateService by inject()
    protected val activeActivityTracker: ActiveActivityTrackingService by inject()

    protected lateinit var drawerLayout: DrawerLayout
    protected lateinit var toolbar: Toolbar
    protected lateinit var navView: NavigationView

    @Suppress("UNUSED_PARAMETER")
    private fun onStateChange(o: Observable, arg: Any?) {
        stateService.getState().updateNavigationView(navView.menu)
        stateService.getState().finishActivityIfNeeded(this)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        volumeControlStream = AudioManager.STREAM_MUSIC
        setContentView(layoutRes)
        drawerLayout = findViewById(drawerLayoutId)
        toolbar = drawerLayout.findViewWithTag(getString(R.string.tag_toolbar))
        navView = drawerLayout.findViewWithTag(getString(R.string.tag_nav_view))

        setSupportActionBar(toolbar)

        val toggle = ActionBarDrawerToggle(
            this, drawerLayout, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close)
        drawerLayout.addDrawerListener(toggle)
        toggle.syncState()

        stateService.addObserver(Observer(this::onStateChange))
        navView.setNavigationItemSelectedListener(this)
        stateService.getState().updateNavigationView(navView.menu)
        stateService.getState().finishActivityIfNeeded(this@AbstractDrawerActivity)
    }

    override fun onStart() {
        super.onStart()
        activeActivityTracker.activityStarted()
    }

    override fun onStop() {
        super.onStop()
        activeActivityTracker.activityStopped()
    }

    override fun onBackPressed() {
        if (drawerLayout.isDrawerOpen(GravityCompat.START)) {
            drawerLayout.closeDrawer(GravityCompat.START)
        } else {
            super.onBackPressed()
        }
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        // Handle navigation view item clicks here.
        when (item.itemId) {
            R.id.nav_queue      -> {
                val intent = Intent(this, QueueActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
                startActivity(intent)
            }
            R.id.nav_local_song -> {
                val intent = Intent(this, LocalSongActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
                startActivity(intent)
            }
            R.id.nav_statistics -> {
                val intent = Intent(this, StatisticsActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
                startActivity(intent)
            }
            R.id.nav_blacklist  -> {
                val intent = Intent(this, BlackListActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
                startActivity(intent)
            }
            R.id.nav_settings   -> {
                val intent = Intent(this, SettingActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT)
                startActivity(intent)
            }
        }

        drawerLayout.closeDrawer(GravityCompat.START)
        return false
    }
}