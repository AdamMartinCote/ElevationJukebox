package ca.polymtl.inf3990_01.client.controller.event

class AppStopEvent: Event {
    companion object {
        val EVENT_TYPE = "app_stop"
    }
    override val type = EVENT_TYPE
}