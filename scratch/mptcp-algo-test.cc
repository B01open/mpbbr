
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/flow-monitor-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/packet-sink.h"
#include "ns3/simulator.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traffic-control-module.h"
#include "ns3/mptcp-socket-base.h"

#include "ns3/enum.h"
#include "ns3/config-store-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"


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

Ptr<Node> client;
Ptr<Node> server;

//yqy
static void
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
  ///*stream->GetStream () << oldval << " " << newval << std::endl;
	 *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}

static void
TraceCwnd (std::string cwndTrFileName)
{
  AsciiTraceHelper ascii;
  if (cwndTrFileName.compare ("") == 0)
    {
      NS_LOG_DEBUG ("No trace file for cwnd provided");
      return;
    }
  else
    {
      Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (cwndTrFileName.c_str ());
      Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",MakeBoundCallback (&CwndTracer, stream));
    }
}

static void
RttTracer (Ptr<OutputStreamWrapper>rttStream, Time oldval, Time newval)
{
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  if (rtt_tr_file_name.compare ("") == 0)
    {
      NS_LOG_DEBUG ("No trace file for rtt provided");
      return;
    }

  Ptr<OutputStreamWrapper>  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeBoundCallback (&RttTracer, rttStream));
}

int
main (int argc, char *argv[])
{
	LogComponentEnable("FirstMultipathToplogy", LOG_LEVEL_ALL);
	// TCP的拥塞控制
	  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpBic"));  //TcpBbr   TcpVegas  TcpBic
	  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));

	  //ECN	  //SACN
	  Config::SetDefault ("ns3::TcpSocketBase::EcnMode",EnumValue (0));  //0 noECN    1 classECN
	  Config::SetDefault ("ns3::TcpSocketBase::Sack",BooleanValue (true));

	  //Enable MPTCP 配置mptcp头部，路径管理模式，自流数量
	  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (false));
	  Config::SetDefault("ns3::MpTcpSocketBase::PathManagerMode", EnumValue (MpTcpSocketBase::nDiffPorts));
	  Config::SetDefault ("ns::MpTcpNdiffPorts::MaxSubflows", UintegerValue (2));

	 double start_time = 0.01;
	 double stop_time = 30;
	 std::string transport_prot = "TcpBic";    //TcpBic     //TcpBbr

	 // Creation of the hosts
	    NodeContainer nodes;
	    nodes.Create(2);
	    client = nodes.Get(0);
	    server = nodes.Get(1);

	    InternetStackHelper stack;
	    stack.Install (nodes);

	  //  vector<Ipv4InterfaceContainer> ipv4Ints;
	   // Ipv4InterfaceContainer ipv4Ints;

	    PointToPointHelper p2plink1;
	    p2plink1.SetDeviceAttribute ("DataRate", StringValue("500Kbps"));
	    p2plink1.SetChannelAttribute("Delay", StringValue("5ms"));
	    NetDeviceContainer netDevices1;
	    netDevices1 = p2plink1.Install(nodes);

	    Ipv4InterfaceContainer interface1;
	    Ipv4InterfaceContainer interface2;
	    Ipv4AddressHelper ipv4addr;
	    ipv4addr.SetBase("10.1.1.0", "255.255.255.0","0.0.0.5");

	    Ipv4AddressHelper ipv4addr1;
	    ipv4addr1.SetBase("10.1.1.0", "255.255.255.0","0.0.0.1");
/*
	    Ptr<NetDevice> device = netDevices1.Get (0);
	    Ptr<Ipv4> ipv4 = client->GetObject<Ipv4> ();
	    int32_t interface = ipv4->GetInterfaceForDevice (device);
	    if (interface == -1)
	           {
	             interface = ipv4->AddInterface (device);
	           }
	    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0"));
	    ipv4->AddAddress (interface, ipv4Addr);
	    ipv4->SetMetric (interface, 1);
	    ipv4->SetUp (interface);
	    interface1.Add (ipv4, interface);


	    Ptr<NetDevice> deviceServer = netDevices1.Get (1);
	    Ptr<Ipv4> ipv4Server = server->GetObject<Ipv4> ();
	    int32_t interfaceServer = ipv4Server->GetInterfaceForDevice (deviceServer);
	    if (interfaceServer == -1)
	           {
	    	interfaceServer = ipv4->AddInterface (device);
	           }
	    Ipv4InterfaceAddress ipv4AddrServer = Ipv4InterfaceAddress (Ipv4Address("10.1.1.2"), Ipv4Mask("255.255.255.0"));
	    ipv4Server->AddAddress (interfaceServer, ipv4AddrServer);
	    ipv4Server->SetMetric (interfaceServer, 1);
	    ipv4Server->SetUp (interfaceServer);
	    interface1.Add (ipv4Server, interfaceServer);
*/

	    interface1 = ipv4addr.Assign(netDevices1);
	    interface2 = ipv4addr1.Assign(netDevices1);
	   // ipv4Ints.Add(ipv4Ints.end(), interface1);

//	    NetDeviceContainer netDevices2;
//	    netDevices2 = p2plink1.Install(nodes);
//	    Ipv4AddressHelper ipv4addr1;
//	    Ipv4InterfaceContainer interface2;
//	    ipv4addr1.SetBase("10.1.1.0", "255.255.255.0","0.0.0.3");
//	    interface2 = ipv4addr1.Assign(netDevices2);
//
//	    ipv4Ints.Add(interface2);
	    //ipv4Ints.insert(ipv4Ints.end(), interface1);
/*
	    PointToPointHelper p2plink2;
	    p2plink2.SetDeviceAttribute ("DataRate", StringValue("500Kbps"));
	    p2plink2.SetChannelAttribute("Delay", StringValue("5ms"));
	    NetDeviceContainer netDevices2;
	    netDevices2 = p2plink2.Install(nodes);

	    Ipv4AddressHelper ipv4addr2;
	    ipv4addr2.SetBase("10.1.2.0", "255.255.255.0");
	    Ipv4InterfaceContainer interface2 = ipv4addr2.Assign(netDevices2);

	    ipv4Ints.Add(interface2);
	    //ipv4Ints.insert(ipv4Ints.end(), interface2);
*/
	    std::cout<<interface1.GetAddress(0)<<std::endl;
	    std::cout<<interface1.GetAddress(1)<<std::endl;
	  //  std::cout<<interface2.GetAddress(2)<<std::endl;
	  //  std::cout<<interface2.GetAddress(3)<<std::endl;

	   uint32_t servPort = 5000;
	   uint32_t maxBytes = 0;
	   BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (interface1.GetAddress(1), servPort));
	   source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

	   /*
	   OnOffHelper oo = OnOffHelper ("ns3::TcpSocketFactory",InetSocketAddress (interface1.GetAddress (1), servPort));
	   oo.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));
	   oo.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
	   oo.SetAttribute ("DataRate", DataRateValue (DataRate ("500kb/s")));
	   */
	   ApplicationContainer SourceApp = source.Install (client);
	   SourceApp.Start (Seconds (start_time));
	   SourceApp.Stop (Seconds (stop_time-1));

	   // Create a packet sink to receive packets.
	   PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (),servPort));   ///在构造期间设置的属性。
	   ApplicationContainer SinkApp = packetSinkHelper.Install (server);
	   SinkApp.Start (Seconds (start_time));
	   SinkApp.Stop (Seconds (stop_time));

	   /****************开启pcap跟踪*******************/
	   p2plink1.EnablePcapAll ("third1", true);

	   std::string dir = "results/" + transport_prot.substr(0, transport_prot.length()) + "/" ;
	   std::string dirToSave = "mkdir -p " + dir;
	   system (dirToSave.c_str ());
	   Simulator::Schedule (Seconds (start_time + 0.1), &TraceCwnd, dir + "cwnd.data");
	   Simulator::Schedule (Seconds (start_time + 0.1), &TraceRtt, dir + "rtt.data");

	   // 8. Install FlowMonitor on all nodes
	   FlowMonitorHelper flowmon;
	   Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

	   Simulator::Stop (Seconds(stop_time + 5.0));
	   Simulator::Run ();
	   NS_LOG_INFO ("Done.");

	   double total = 0;
	   monitor->CheckForLostPackets ();
	   Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	   FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
	   for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	   {
	       Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	       std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
	       std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
	       std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
	       std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 1000 / 1000  << " Mbps\n";
	       std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
	       std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
	       std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 1000 / 1000  << " Mbps\n";

	       total +=  i->second.rxBytes * 8.0 / 1000 / 1000;
	  }
	  std::cout<<"tatal = "<<" "<<total<<std::endl;

	   std::cout << "Simulation finished "<<"\n";
	   Simulator::Destroy ();

	   Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (SinkApp.Get (0));
	   std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
