#include "DaemonMaster.hpp"
#include "signal/SignalHandling.hpp"
#include "communication/ListenerSocket.hpp"
#include "communication/ClientSocket.hpp"
#include "ssl/SslContext.hpp"
#include "core/DaemonRunner.hpp"

namespace elevation {
namespace daemon {

DaemonMaster::DaemonMaster(const ArgsParser::Config& config)
    : m_config(config)
{ }

void DaemonMaster::run() {
    SslContext::createInstance(m_config.getListenPort());
    SslContext& ctx = SslContext::getInstance();

    while (true) {
        try {
            SslSession session = ctx.acceptSession();
            ClientSocket httpServerSocket(m_config.getOutputPort());

            DaemonRunner runner(std::move(session), std::move(httpServerSocket));

            if (SignalHandling::s_cleanupRequested.load()) {
                throw std::runtime_error("SSL Daemon killed by signal.");
            }
        }
        catch (const std::exception& e) {
            std::cerr << "A C++ exception occurred while trying to establish client-server connections : " <<
                e.what() << std::endl;
        }
    }
}

} // namespace daemon
} // namespace elevation