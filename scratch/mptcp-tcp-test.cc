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
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  LogComponentEnable ("BottleNeckTcpScriptExample", LOG_LEVEL_INFO);
 // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("OnOff", LOG_LEVEL_ALL);

  double minRto = 0.02;
  uint32_t initialCwnd = 7;
  uint32_t mtu_bytes = 536;
  double start_time = 0.01;
  bool tracing = false;
  double stop_time = 15;
  double error_p = 0.00;
  int sfNum = 2;
  bool enableMptcp = true;
  bool enableSack = true;
  int enableEcn = 0;
  std::string transport_prot = "ns3::TcpBbr";    //TcpBbr   TcpBic  TcpVegas

  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "1ms";
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
 Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
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
  nodes.Create (3);

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  NodeContainer n0n2 = NodeContainer(nodes.Get(0),nodes.Get(2));
  NodeContainer n1n2 = NodeContainer(nodes.Get(1),nodes.Get(2));
  NodeContainer n11n2 = NodeContainer(nodes.Get(1),nodes.Get(2));

  InternetStackHelper internet;
  internet.Install(nodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay",StringValue("0.2ms"));
 // p2p.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d0d2 =p2p.Install(n0n2);

 // PointToPointHelper r2r;
  p2p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay",StringValue("0.2ms"));
 // p2p.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d1d2 =p2p.Install(n1n2);
  NetDeviceContainer d11d2 =p2p.Install(n11n2);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0","255.255.255.0");
  Ipv4InterfaceContainer i0i2 = ipv4.Assign(d0d2);

  ipv4.SetBase("10.1.2.0","255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(d1d2);

  ipv4.SetBase("10.1.3.0","255.255.255.0");
  ipv4.Assign(d11d2);

  uint16_t port = 50000;
  uint16_t udpport = 40000;
  ApplicationContainer sinkApp;
  ApplicationContainer clientApps;

  //server 1
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkApp.Add(sinkHelper.Install(nodes.Get(2)));

  //server 2
  PacketSinkHelper udpSink ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), udpport)));
  sinkApp.Add (udpSink.Install (nodes.Get(2)));

  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time + 1));

  //client 1
  BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (i0i2.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  source.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  clientApps = source.Install (nodes.Get(0));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time));

  //client 2
  AddressValue remoteAddress (InetSocketAddress (i0i2.GetAddress (1), port));
  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", Address ());
  onOffHelper.SetConstantRate (DataRate ("1Mbps"));
  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("Remote", remoteAddress);
  clientApps = onOffHelper.Install (nodes.Get(4));
  clientApps.Start (Seconds (start_time));
  clientApps.Stop (Seconds (stop_time - 1));

//------------------------------------------------------

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //嗅探,记录所有节点相关的数据包

  if (tracing)
    {
      AsciiTraceHelper ascii;
      p2p.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      p2p.EnablePcapAll ("tcp-bulk-send", false);
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

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApp.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  return 0;
}

