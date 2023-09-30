#include "protocols.h"

RMT_PROTOCOL::RMT_PROTOCOL() {
   
}

void RMT_PROTOCOL::registerProtocol(rmt_protocol_t protocol) {
    customProtocols[activeCustomProtocols] = protocol;
    activeCustomProtocols++;
}