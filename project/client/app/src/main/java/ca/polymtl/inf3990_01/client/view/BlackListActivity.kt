package ca.polymtl.inf3990_01.client.view

import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import ca.polymtl.inf3990_01.client.R
import ca.polymtl.inf3990_01.client.controller.state.AppStateService

class BlackListActivity : AbstractDrawerActivity(R.layout.activity_black_list, R.id.drawer_layout) {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        menuInflater.inflate(R.menu.black_list, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        when (item.itemId) {
            R.id.action_change_state -> {
                val currState = stateService.getState().type
                val v = AppStateService.State.values()
                stateService.setState(v[(v.indexOf(currState) + 1) % v.size])
                return true
            }
            R.id.action_settings -> return true
            else -> return super.onOptionsItemSelected(item)
        }
    }
}