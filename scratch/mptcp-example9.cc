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

/*
 	  ---r0--------r2--
	/ 	  \   	  / 	\
  /			\	/		  \
c  			  \	    		s
  \			/	\		  /
	\	  /		   \	/
	  ---r1--------r3--
 * */


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MpTcpTestingExample");

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

   //LogComponentEnable("TcpBbr", LOG_LEVEL_ALL);
 //  LogComponentEnable("MpTcpTestingExample", LOG_LEVEL_ALL);
  // LogComponentEnable("MpTcpSocketBase", LOG_DEBUG);
  // LogComponentEnable("MpTcpSocketBase", LOG_LOGIC);
 //  LogComponentEnable("MpTcpSocketBase", LOG_ERROR);
  // LogComponentEnable("TcpSocketBase", LOG_ERROR);


   double minRto = 0.2;
   uint32_t initialCwnd = 10;
   uint32_t mtuSize = 1500;
   double start_time = 0.01;
   double stop_time = 30;
   std::string transport_prot = "TcpBbr";

  // TCP的拥塞控制
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpBbr"));   //TcpBbr   TcpBic

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (mtuSize));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (initialCwnd));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (minRto)));
 
  //Enable MPTCP 配置mptcp头部，路径管理模式，自流数量
  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (true));
  Config::SetDefault("ns3::MpTcpSocketBase::PathManagerMode", EnumValue (MpTcpSocketBase::FullMesh));
  Config::SetDefault ("ns::MpTcpNdiffPorts::MaxSubflows", UintegerValue (2));

  //ECN     //SACN
  Config::SetDefault ("ns3::TcpSocketBase::EcnMode",EnumValue (1));  //0 noECN    1 classECN
  Config::SetDefault ("ns3::TcpSocketBase::Sack",BooleanValue (true));

// Variables Declaration 端口及最大字节数
   uint16_t port = 999;
   uint32_t maxBytes = 0;
 
// Initialize Internet Stack and Routing Protocols 初始化协议栈和路由
	InternetStackHelper internet;
	Ipv4AddressHelper ipv4;

// creating routers, source and destination. Installing internet stack 创建路由，源，目的端口及安装协议栈
  NodeContainer router;				// NodeContainer for router	
		router.Create (6);
		internet.Install (router);	
  NodeContainer host;				// NodeContainer for source and destination	
		host.Create (2);
		internet.Install (host);

    Ptr<Node> client = host.Get (0);
    Ptr<Node> server = host.Get (1);


/////////////////////创建拓扑////////////////
// connecting routers and hosts and assign ip addresses 连接路由和主机，并分配ip

  NodeContainer h0r0 = NodeContainer (client, router.Get (0));
  NodeContainer h0r1 = NodeContainer (client, router.Get (1));
  NodeContainer h0r2 = NodeContainer (client, router.Get (2));
  
  NodeContainer r0r3 = NodeContainer (router.Get (0), router.Get (3));
  NodeContainer r0r4 = NodeContainer (router.Get (0), router.Get (4));
  NodeContainer r0r5 = NodeContainer (router.Get (0), router.Get (5));

  NodeContainer r1r3 = NodeContainer (router.Get (1), router.Get (3));
  NodeContainer r1r4 = NodeContainer (router.Get (1), router.Get (4));
  NodeContainer r1r5 = NodeContainer (router.Get (1), router.Get (5));

  NodeContainer r2r3 = NodeContainer (router.Get (2), router.Get (3));
  NodeContainer r2r4 = NodeContainer (router.Get (2), router.Get (4));
  NodeContainer r2r5 = NodeContainer (router.Get (2), router.Get (5));

  NodeContainer h1r3 = NodeContainer (server, router.Get (3));
  NodeContainer h1r4 = NodeContainer (server, router.Get (4));
  NodeContainer h1r5 = NodeContainer (server, router.Get (5));

  //NodeContainer r0r1 = NodeContainer (router.Get (0), router.Get (1));
// We create the channels first without any IP addressing information 创建管道及连接
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
 
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  NetDeviceContainer l0h0r0 = p2p.Install (h0r0);
  NetDeviceContainer l1h0r1 = p2p.Install (h0r1);
  NetDeviceContainer l2h0r2 = p2p.Install (h0r2);


  p2p.SetDeviceAttribute ("DataRate", StringValue ("2.4Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer l3r0r3 = p2p.Install (r0r3);
  NetDeviceContainer l4r0r4 = p2p.Install (r0r4);
  NetDeviceContainer l5r0r5 = p2p.Install (r0r5);

  NetDeviceContainer l6r1r3 = p2p.Install (r1r3);
  NetDeviceContainer l7r1r4 = p2p.Install (r1r4);
  NetDeviceContainer l8r1r5 = p2p.Install (r1r5);

  NetDeviceContainer l9r2r3 = p2p.Install (r2r3);
  NetDeviceContainer l10r2r4 = p2p.Install (r2r4);
  NetDeviceContainer l11r2r5 = p2p.Install (r2r5);
 
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  NetDeviceContainer l12h1r2 = p2p.Install (h1r3);
  NetDeviceContainer l13h1r3 = p2p.Install (h1r4);
  NetDeviceContainer l14h1r3 = p2p.Install (h1r5);
  
// Later, we add IP addresses. 分配ip
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0h0r0 = ipv4.Assign (l0h0r0);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1h0r1 = ipv4.Assign (l1h0r1);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i2r0r2 = ipv4.Assign (l2h0r2);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3r0r3 = ipv4.Assign (l3r0r3);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i5r1r3 = ipv4.Assign (l4r0r4);

  ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i6h1r2 = ipv4.Assign (l5r0r5);

  ipv4.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i7h1r3 = ipv4.Assign (l6r1r3);

  ipv4.SetBase ("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i8h1r3 = ipv4.Assign (l7r1r4);

  ipv4.SetBase ("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i9h1r3 = ipv4.Assign (l8r1r5);

  ipv4.SetBase ("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i10h1r3 = ipv4.Assign (l9r2r3);

  ipv4.SetBase ("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i11h1r3 = ipv4.Assign (l10r2r4);

  ipv4.SetBase ("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i12h1r3 = ipv4.Assign (l11r2r5);

  ipv4.SetBase ("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i13h1r3 = ipv4.Assign (l12h1r2);

  ipv4.SetBase ("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i14h1r3 = ipv4.Assign (l13h1r3);
  
  ipv4.SetBase ("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer i15h1r3 = ipv4.Assign (l14h1r3);

// Create router nodes, initialize routing database and set up the routing
  // tables in the nodes. 建立路由
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  NS_LOG_INFO ("Create applications.");
   
  // Create OnOff Application

  // Create BuldSend Application to send Tcp packets  
  BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (i13h1r3.GetAddress (0), port));
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

/*
  OnOffHelper oo = OnOffHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (i6h1r2.GetAddress (0), port)); //link from router1 to host1
  // Set the amount of data to send in bytes.  Zero is unlimited.
  oo.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  oo.SetAttribute ("PacketSize", UintegerValue (1000));
  oo.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.02]"));
  oo.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));
  oo.SetAttribute ("DataRate", DataRateValue (DataRate ("500kb/s")));
*/

  ApplicationContainer SourceApp = source.Install (client);
  SourceApp.Start (Seconds (start_time));
  SourceApp.Stop (Seconds (stop_time-1));
  
  // Create a packet sink to receive packets.
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer SinkApp = packetSinkHelper.Install (server);
  SinkApp.Start (Seconds (start_time));
  SinkApp.Stop (Seconds (stop_time));
                                        
//=========== Start the simulation ===========//

	std::cout << "Start Simulation.. "<<"\n";

  ////////////////////////// Use of NetAnim model/////////////////////////////////////////
	/*
   AnimationInterface anim ("mptcp-example.xml");
	anim.SetConstantPosition(router.Get(0), 60.0, 20.0);
    anim.SetConstantPosition(router.Get(1), 60.0, 0.0);
    anim.SetConstantPosition(router.Get(2), 120.0, 20.0);
    anim.SetConstantPosition(router.Get(3), 120.0, 0.0);

    anim.SetConstantPosition(host.Get(0), 2.0, 0.0);
    anim.SetConstantPosition(host.Get(1), 200.0 , 0.0);  
	anim.UpdateNodeDescription (router.Get(0), "router-point-0");
    anim.UpdateNodeDescription (router.Get(1), "router-point-1");
    anim.UpdateNodeDescription (router.Get(2), "router-point-2");
    anim.UpdateNodeDescription (router.Get(3), "router-point-3");
    anim.UpdateNodeDescription (host.Get(0), "send-point");
    anim.UpdateNodeDescription (host.Get(1), "receive-point");
*/
/////////////////////////////////////////////////////////////////////////////////////////////
       // Output config store to txt format 输出文件存储
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();

  NS_LOG_INFO ("Run Simulation.");

  //yqy
  std::string dir = "results/" + transport_prot.substr(0, transport_prot.length()) + "/" ;
  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  Simulator::Schedule (Seconds (start_time + 0.1), &TraceCwnd, dir + "cwnd.data");
  Simulator::Schedule (Seconds (start_time + 1), &TraceRtt, dir + "rtt.data");

  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  NS_LOG_INFO("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  Simulator::Stop (Seconds(stop_time));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");
      
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
    }

  std::cout << "Simulation finished "<<"\n";
  NS_LOG_INFO("finishing flow at time " <<  Simulator::Now ().GetSeconds ());
  Simulator::Destroy ();
  	
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (SinkApp.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
	return 0;
}




