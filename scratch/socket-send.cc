#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdint.h>
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/log.h"

/* Multipath Network Topology

     lan 10.1.1.0
      ___________
     /           \
   n1             n2
     \___________/

     lan 10.1.2.0
*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstMultipathToplogy");

// The number of bytes to send in this simulation.
static const uint32_t totalTxBytes = 0xFFFFFFFF;
//static const uint32_t sendBufSize  = 180000; //2000000;
//static const uint32_t recvBufSize  = 100000; //2000000;
static uint32_t currentTxBytes     = 0;
static const double simDuration    = 100.0;

Ptr<Node> client;
Ptr<Node> server;


// Perform series of 1040 byte writes (this is a multiple of 26 since
// we want to detect data splicing in the output stream)
static const uint32_t writeSize = 1500;//sendBufSize;
uint8_t data[writeSize];
Ptr<MpTcpSocketBase> lSocket    = 0;
Ptr<TcpL4Protocol>  m_tcp;

void StartFlow(Ptr<MpTcpSocketBase> , Ipv4Address, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, unsigned int);

int main(int argc, char *argv[])
{
	TcpSocketBase socketBase;

//	socketBase.m_congestionControl = 1;

	for(uint32_t i = 0; i < writeSize; ++i)
    {
      char m = toascii (97 + i % 26);
      data[i] = m;
    }

    // Creation of the hosts
    NodeContainer nodes;
    nodes.Create(2);
    client = nodes.Get(0);
    server = nodes.Get(1);

    InternetStackHelper stack;
    stack.SetTcp ("ns3::MpTcpL4Protocol");
    stack.Install (nodes);

    Ipv4InterfaceContainer ipv4Ints;

    // Creation of the point to point link between hots
    PointToPointHelper p2plink;

	p2plink.SetDeviceAttribute ("DataRate", StringValue("1Mbps"));
	p2plink.SetChannelAttribute("Delay", StringValue("100ms"));
		
    NetDeviceContainer netDevices;
    netDevices = p2plink.Install(nodes);

    Ipv4AddressHelper ipv4addr;
    ipv4addr.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interface = ipv4addr.Assign(netDevices);

    // Configuration of the Client/Server application
    uint32_t servPort = 5000;
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",InetSocketAddress (ipv4Ints.GetAddress (1), servPort));
    ApplicationContainer SinkApp = packetSinkHelper.Install (server);
    ApplicationContainer Apps;
    Apps.Add(SinkApp);
    Apps.Start(Seconds(0.0));
    Apps.Stop(Seconds(simDuration));

    Simulator::ScheduleNow (&StartFlow, lSocket, ipv4Ints.GetAddress (0), ipv4Ints.GetAddress (1), servPort);

    Simulator::Stop (Seconds(simDuration + 10.0));
    Simulator::Run ();
    Simulator::Destroy();

    NS_LOG_LOGIC("mpTopology:: simulation ended");
    return 0;
}

//-----------------------------------------------------------------------------
//begin implementation of sending "Application"
void StartFlow(Ptr<MpTcpSocketBase> lSocket, Ipv4Address srcAddress, Ipv4Address dstAddress, uint16_t servPort)
{
  NS_LOG_LOGIC("Starting flow at time " <<  Simulator::Now ().GetSeconds ());

  int connectionState = lSocket->ConnectNewSubflow(srcAddress, dstAddress);
  Ptr<Socket> skt = DynamicCast<Socket>(lSocket);

  if(connectionState == 0)
  {
	  skt->SetDataSentCallback (MakeCallback (&WriteUntilBufferFull));
	  WriteUntilBufferFull(skt, skt->GetTxAvailable ());
  }else{
	  NS_LOG_LOGIC("socket-send.cc:: connection failed");
  }
}

void WriteUntilBufferFull (Ptr<Socket> skt, unsigned int txSpace)
{
    NS_LOG_FUNCTION_NOARGS();

  while (currentTxBytes < totalTxBytes && skt->GetTxAvailable () > 0)
  {
      uint32_t left    = totalTxBytes - currentTxBytes;
      uint32_t toWrite = std::min(writeSize, txSpace);
      toWrite = std::min( toWrite , left );
      uint32_t dataOffset = currentTxBytes % writeSize;
     //NS_LOG_LOGIC("mpTopology:: data already sent ("<< currentTxBytes <<") data buffered ("<< toWrite << ") to be sent subsequentlly");
      int amountBuffered = skt->Send (&data[dataOffset], toWrite, 0);
    //  lSocket->SendPendingData(true);
      if(amountBuffered < 0){
    	  return;
      }
      currentTxBytes += amountBuffered;
  }
  skt->Close ();
}


