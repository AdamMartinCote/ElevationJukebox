package ca.polymtl.inf3990_01.client.view

import android.app.Dialog
import android.content.Context
import android.view.inputmethod.EditorInfo
import ca.polymtl.inf3990_01.client.R
import ca.polymtl.inf3990_01.client.controller.event.EventManager
import ca.polymtl.inf3990_01.client.controller.event.LoginRequestEvent
import kotlinx.android.synthetic.main.login_layout.*

class LoginDialog(
    activityCtx: Context,
    private val eventManager: EventManager
): Dialog(activityCtx, R.style.Dialog) {
    private var canceled: Boolean = false

    init {
        setTitle(R.string.login_dialog_title)
        setCancelable(true)
        setCanceledOnTouchOutside(true)
        setOnCancelListener { canceled = true; this@LoginDialog.dismiss() }
        setContentView(R.layout.login_layout)
        login_username.setText(R.string.login_username_default_text)
        login_username.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                login_password.requestFocus()
                true
            }
            else {
                false
            }
        }
        login_password.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                login_button_submit.performClick()
            }
            else {
                false
            }
        }
        login_button_submit.setOnClickListener {
            this@LoginDialog.dismiss()
        }
        setOnDismissListener {
            if (!canceled) {
                eventManager.dispatchEvent(LoginRequestEvent(login_username.text.toString(), login_password.text.toString()))
            }
        }
    }
}