package ca.polymtl.inf3990_01.client.view

import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import ca.polymtl.inf3990_01.client.R
import ca.polymtl.inf3990_01.client.controller.event.EventManager
import ca.polymtl.inf3990_01.client.controller.event.RequestBlackListReloadEvent
import ca.polymtl.inf3990_01.client.controller.state.AppStateService
import ca.polymtl.inf3990_01.client.model.UserList
import ca.polymtl.inf3990_01.client.presentation.BlackListAdapter
import kotlinx.android.synthetic.main.content_black_list.*
import org.koin.android.ext.android.inject
import org.koin.core.parameter.ParameterList

class BlackListActivity : AbstractDrawerActivity(R.layout.activity_black_list, R.id.drawer_layout) {

    var usersList = UserList()
    val eventMgr: EventManager by inject()
    val blackListAdapter: BlackListAdapter by inject{ ParameterList(usersList, layoutInflater) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        users_list.adapter = blackListAdapter
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
            R.id.action_reload ->
            { eventMgr.dispatchEvent(RequestBlackListReloadEvent())
                return true }
            else -> return super.onOptionsItemSelected(item)
        }
    }
}

