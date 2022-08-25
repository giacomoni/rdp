//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef COMMON_QUEUE_RDPMARKINGSWITCHQUEUE_H_
#define COMMON_QUEUE_RDPMARKINGSWITCHQUEUE_H_

#include <inet/queueing/base/PacketQueueBase.h>
#include <inet/queueing/contract/IPacketBuffer.h>
#include <inet/queueing/contract/IPacketCollection.h>
#include <inet/queueing/contract/IPassivePacketSink.h>
#include <inet/queueing/contract/IPacketComparatorFunction.h>
#include <inet/queueing/contract/IPacketDropperFunction.h>
#include <inet/queueing/contract/IPassivePacketSource.h>
#include <inet/queueing/contract/IActivePacketSink.h>
#include <inet/queueing/contract/IActivePacketSource.h>
#include <inet/common/INETDefs.h>

#include "RdpSwitchQueue.h"

namespace inet {
namespace queueing {
/**
 * Drop-front queue. See NED for more info.
 */
class INET_API RdpMarkingSwitchQueue : public RdpSwitchQueue
{
  protected:
    virtual void initialize(int stage) override;
  public:
    virtual void pushPacket(Packet *packet, cGate *gate) override;
  protected:
    int kthresh;
};

}
} // namespace inet

#endif // ifndef __INET_RdpSwitchQueue_H
