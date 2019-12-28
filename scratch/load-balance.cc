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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BottleNeckTcpScriptExample");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("BottleNeckTcpScriptExample", LOG_LEVEL_INFO);
 // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("OnOff", LOG_LEVEL_ALL);

  bool tracing = false;
  double minRto = 0.02;
  uint32_t initialCwnd = 7;
  uint32_t mtu_bytes = 536;
  double start_time = 0.01;
  double stop_time = 25;
  double error_p = 0.00;
  int sfNum = 8;
  bool enableMptcp = true;
  bool enableSack = true;
  int enableEcn = 0;
  std::string transport_prot = "ns3::TcpBbr";    //TcpBbr   TcpBic  TcpVegas

  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "3ms";
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
  nodes.Create (12);

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

  NodeContainer n8n1 = NodeContainer(nodes.Get(8),nodes.Get(1));
  NodeContainer n1n5 = NodeContainer(nodes.Get(1),nodes.Get(5));
  NodeContainer n2n9 = NodeContainer(nodes.Get(2),nodes.Get(9));
  NodeContainer n6n9 = NodeContainer(nodes.Get(6),nodes.Get(9));

  NodeContainer n5n10 = NodeContainer(nodes.Get(5),nodes.Get(10));
  NodeContainer n6n11 = NodeContainer(nodes.Get(6),nodes.Get(11));


  InternetStackHelper internet;
  internet.Install(nodes);

  PointToPointHelper p2ptx1;
  p2ptx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2ptx1.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d0d1 =p2ptx1.Install(n0n1);
  //tchPfifoFastAccess.Install(d3d4);

  PointToPointHelper p2ptx2;
  p2ptx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2ptx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d4d5 =p2ptx2.Install(n4n5);
  //tchPfifoFastAccess.Install(d3d4);

  PointToPointHelper p2ptx3;
  p2ptx3.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2ptx3.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d5d10 =p2ptx3.Install(n5n10);
  //tchPfifoFastAccess.Install(d5d10);

  PointToPointHelper p2pmptx1;
  p2pmptx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2pmptx1.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d8d1 =p2pmptx1.Install(n8n1);
  //tchPfifoFastAccess.Install(d3d4);

  PointToPointHelper p2pmptx2;
  p2pmptx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2pmptx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d1d5 =p2pmptx2.Install(n1n5);
  //tchPfifoFastAccess.Install(d3d4);

  PointToPointHelper p2pr1;
  p2pr1.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  p2pr1.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d1d2 =p2pr1.Install(n1n2);

  PointToPointHelper p2pr2;
  p2pr2.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  p2pr2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d5d6 =p2pr2.Install(n5n6);


  PointToPointHelper p2prx1;
  p2prx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx1.SetChannelAttribute("Delay",StringValue("1ms"));
 // p2p.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d2d3 =p2prx1.Install(n2n3);

  PointToPointHelper p2prx2;
  p2prx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d6d7 =p2prx2.Install(n6n7);

  PointToPointHelper p2prx3;
  p2prx3.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx3.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d6d11 =p2prx3.Install(n6n11);

  PointToPointHelper p2pmprx1;
  p2pmprx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2pmprx1.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d2d9 =p2pmprx1.Install(n2n9);

  PointToPointHelper p2pmprx2;
  p2pmprx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2pmprx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d6d9 =p2pmprx2.Install(n6n9);

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
  Ipv4InterfaceContainer i8i1 = ipv4.Assign(d8d1);

  ipv4.SetBase("10.1.8.0","255.255.255.0");
  Ipv4InterfaceContainer i1i5 = ipv4.Assign(d1d5);

  ipv4.SetBase("10.1.9.0","255.255.255.0");
  Ipv4InterfaceContainer i2i9 = ipv4.Assign(d2d9);

  ipv4.SetBase("10.1.10.0","255.255.255.0");
  Ipv4InterfaceContainer i6i9 = ipv4.Assign(d6d9);

  ipv4.SetBase("10.1.11.0","255.255.255.0");
  Ipv4InterfaceContainer i5i10 = ipv4.Assign(d5d10);

  ipv4.SetBase("10.1.12.0","255.255.255.0");
  Ipv4InterfaceContainer i6i11 = ipv4.Assign(d6d11);

  uint16_t port = 50000;
  ApplicationContainer sinkApp1, sinkApp2, sinkApp3;
  ApplicationContainer clientApps, clientApps2, clientApps3, clientApps4;

  //server
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkApp1.Add(sinkHelper.Install(nodes.Get(3)));
  sinkApp1.Add(sinkHelper.Install(nodes.Get(7)));
  sinkApp1.Add(sinkHelper.Install(nodes.Get(9)));
  sinkApp1.Add(sinkHelper.Install(nodes.Get(11)));
  sinkApp1.Start (Seconds (start_time));
  sinkApp1.Stop (Seconds (stop_time));

  //client 1
  BulkSendHelper source1 ("ns3::TcpSocketFactory",InetSocketAddress (i2i3.GetAddress (1), port));
  source1.SetAttribute ("MaxBytes", UintegerValue (0));
  source1.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps = source1.Install (nodes.Get(0));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time));

  //client 2
  BulkSendHelper source2 ("ns3::TcpSocketFactory",InetSocketAddress (i6i7.GetAddress (1), port));
  source2.SetAttribute ("MaxBytes", UintegerValue (0));
  source2.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps2 = source2.Install (nodes.Get(4));
  clientApps2.Start(Seconds(start_time));
  clientApps2.Stop (Seconds (stop_time));

  //client 3
  BulkSendHelper source3 ("ns3::TcpSocketFactory",InetSocketAddress (i6i11.GetAddress (1), port));
  source3.SetAttribute ("MaxBytes", UintegerValue (0));
  source3.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps3 = source3.Install (nodes.Get(10));
  clientApps3.Start(Seconds(start_time));
  clientApps3.Stop (Seconds (stop_time));

  //mp client 4
  BulkSendHelper source4 ("ns3::TcpSocketFactory",InetSocketAddress (i2i9.GetAddress (1), port));
  source4.SetAttribute ("MaxBytes", UintegerValue (0));
  source4.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps4 = source4.Install (nodes.Get(8));
  clientApps4.Start(Seconds(start_time));
  clientApps4.Stop (Seconds (stop_time));
//------------------------------------------------------

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //嗅探,记录所有节点相关的数据包

  if (tracing)
    {
    //  AsciiTraceHelper ascii;
      //p2prx1.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      p2prx1.EnablePcapAll ("tcp-bulk-send", false);
    }

  //
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds(stop_time + 1));
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

 // Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApp.Get (0));
 // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  return 0;
}


