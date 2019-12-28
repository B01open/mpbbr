/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
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

#include "ns3/gtk-config-store.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("topology-TEST");

bool m_state_flag2 = false;
static void ChangeDataRate2 ()
{
  if (m_state_flag2)
    {
      Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      Config::Set ("/NodeList/*/DeviceList/1/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      m_state_flag2 = false;
    }
  else
    {
	   Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("10Mbps"));
	   Config::Set ("/NodeList/*/DeviceList/1/$ns3::PointToPointNetDevice/DataRate", StringValue ("10Mbps"));
      m_state_flag2 = true;
    }
    Simulator::Schedule (Seconds (15), &ChangeDataRate2);
}

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("topology-TEST", LOG_LEVEL_INFO);

  // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
 // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);

  bool tracing = true;
  double minRto = 0.02;
  uint32_t initialCwnd = 7;
  uint32_t mtu_bytes = 536;
  double start_time = 0.01;
  double stop_time = 30;

  double error_p = 0.00;
  int sfNum = 8;
  bool enableMptcp = true;
  bool enableSack = true;
  int enableEcn = 0;
  std::string transport_prot = "ns3::TcpBbr";    //TcpBbr   TcpBic  TcpVegas
  std::string scenario = "1";

  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "10ms";
  uint32_t size  = 3;

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  delete temp_header;

  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);

  DataRate access_b (access_bandwidth);
  Time access_d (access_delay);

  size *= (access_b.GetBitRate () / 8) * (access_d * 2).GetSeconds ();
  stringstream strValue;
  strValue << (size / tcp_adu_size);
  TrafficControlHelper tchPfifoFastAccess;
  tchPfifoFastAccess.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue (strValue.str() + "p"));

 // TCP的拥塞控制
 //Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
 Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (transport_prot));

 Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
 Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (initialCwnd));
 Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (minRto)));

 //ECN     //SACN
 Config::SetDefault ("ns3::TcpSocketBase::EcnMode",EnumValue (enableEcn));  //0 noECN    1 classECN
 Config::SetDefault ("ns3::TcpSocketBase::Sack",BooleanValue (enableSack));

 //Enable MPTCP 配置mptcp头部，路径管理模式，自流数量
 Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (enableMptcp));
 Config::SetDefault("ns3::MpTcpSocketBase::PathManagerMode", EnumValue (MpTcpSocketBase::FullMesh));
 Config::SetDefault ("ns::MpTcpNdiffPorts::MaxSubflows", UintegerValue (sfNum));

  NodeContainer nodes;
  nodes.Create (8);

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  NodeContainer n0n1 = NodeContainer(nodes.Get(0),nodes.Get(1));
  NodeContainer n1n2 = NodeContainer(nodes.Get(1),nodes.Get(2));
  NodeContainer n2n3 = NodeContainer(nodes.Get(2),nodes.Get(3));

  NodeContainer n4n5 = NodeContainer(nodes.Get(4),nodes.Get(5));
  NodeContainer n5n6 = NodeContainer(nodes.Get(5),nodes.Get(6));
  NodeContainer n6n7 = NodeContainer(nodes.Get(6),nodes.Get(7));

  NodeContainer n1n5 = NodeContainer(nodes.Get(1),nodes.Get(5));
  NodeContainer n2n6 = NodeContainer(nodes.Get(2),nodes.Get(6));


  InternetStackHelper internet;
  internet.Install(nodes);

  PointToPointHelper mpp2ptx;
  mpp2ptx.SetDeviceAttribute("DataRate",StringValue("12Mbps"));
  mpp2ptx.SetChannelAttribute("Delay",StringValue("3ms"));
  NetDeviceContainer d0d1 =mpp2ptx.Install(n0n1);
  //tchPfifoFastAccess.Install(d0d1);

  PointToPointHelper p2ptx;
  p2ptx.SetDeviceAttribute("DataRate",StringValue("12Mbps"));
  p2ptx.SetChannelAttribute("Delay",StringValue("4ms"));
  NetDeviceContainer d4d5 =p2ptx.Install(n4n5);
  //tchPfifoFastAccess.Install(d4d5);

  PointToPointHelper r1;
  r1.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  r1.SetChannelAttribute("Delay",StringValue("4ms"));
  NetDeviceContainer d1d2 =r1.Install(n1n2);

  PointToPointHelper r2;
  r2.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  r2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d1d5 =r2.Install(n1n5);

  PointToPointHelper r3;
  r3.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  r3.SetChannelAttribute("Delay",StringValue("2ms"));
  NetDeviceContainer d5d6 =r3.Install(n5n6);

  PointToPointHelper r4;
  r4.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  r4.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d2d6 =r4.Install(n2n6);

  PointToPointHelper mpp2prx;
  mpp2prx.SetDeviceAttribute("DataRate",StringValue("12Mbps"));
  mpp2prx.SetChannelAttribute("Delay",StringValue("3ms"));
 // mpp2prx.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d2d3 =mpp2prx.Install(n2n3);

  PointToPointHelper p2prx;
  p2prx.SetDeviceAttribute("DataRate",StringValue("12Mbps"));
  p2prx.SetChannelAttribute("Delay",StringValue("1ms"));
  // p2prx.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d6d7 =p2prx.Install(n6n7);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0","255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign(d0d1);

  ipv4.SetBase("10.1.2.0","255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(d1d2);

  ipv4.SetBase("10.1.3.0","255.255.255.0");
  Ipv4InterfaceContainer i2i3 = ipv4.Assign(d2d3);

  ipv4.SetBase("10.1.4.0","255.255.255.0");
  Ipv4InterfaceContainer i4i5 = ipv4.Assign(d4d5);

  ipv4.SetBase("10.1.5.0","255.255.255.0");
  Ipv4InterfaceContainer i5i6 = ipv4.Assign(d5d6);

  ipv4.SetBase("10.1.6.0","255.255.255.0");
  Ipv4InterfaceContainer i6i7 = ipv4.Assign(d6d7);

  ipv4.SetBase("10.1.7.0","255.255.255.0");
  Ipv4InterfaceContainer i1i5 = ipv4.Assign(d1d5);

  ipv4.SetBase("10.1.8.0","255.255.255.0");
  Ipv4InterfaceContainer i2i6 = ipv4.Assign(d2d6);

  uint16_t port = 50000;
  ApplicationContainer sinkApp;
  ApplicationContainer clientApps, clientApps2;

  //server
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkApp.Add(sinkHelper.Install(nodes.Get(3)));
  sinkApp.Add(sinkHelper.Install(nodes.Get(7)));
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time + 1));


  BulkSendHelper source ("ns3::TcpSocketFactory",Address ());//InetSocketAddress (i1i2.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  source.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));

  /*
  //client 1 mptcp
  AddressValue remoteAddress (InetSocketAddress (i2i3.GetAddress (1), port));
  source.SetAttribute ("Remote", remoteAddress);
  clientApps = source.Install (nodes.Get(0));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time + 1));
*/

  //client 2 tcp
  AddressValue remoteAddress1 (InetSocketAddress (i6i7.GetAddress (1), port));
  source.SetAttribute ("Remote", remoteAddress1);
  clientApps = source.Install (nodes.Get(4));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time));

  /*
  //client 2
  BulkSendHelper sourcetcp ("ns3::TcpSocketFactory",InetSocketAddress (i0i1.GetAddress (0), port));
  sourcetcp.SetAttribute ("MaxBytes", UintegerValue (0));
  sourcetcp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps2 = sourcetcp.Install (nodes.Get(4));
  clientApps2.Start(Seconds(start_time + 5));
  clientApps2.Stop (Seconds (stop_time - 10));
*/
//------------------------------------------------------
  //嗅探,记录所有节点相关的数据包
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::string dir = "results-pcap/";
  std::string dirToSave = "mkdir -p " + dir;
  if (tracing)
  {
	  p2prx.EnablePcapAll ("tcp-bulk-send", false);
  //    mpp2prx.EnablePcapAll (dir + "p", n0n1.Get(0));
  }

  if (scenario == "2")
    {
        Simulator::Schedule (Seconds (5), &ChangeDataRate2);
    }

  //
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds(stop_time + 2));
  Simulator::Run ();

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

  Ptr<PacketSink> sink0 = DynamicCast<PacketSink> (sinkApp.Get (0));
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApp.Get (1));
  std::cout << "sink0 Total Bytes Received: " << sink0->GetTotalRx () << std::endl;
  std::cout << "sink1 Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  return 0;
}

