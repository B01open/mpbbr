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

NS_LOG_COMPONENT_DEFINE ("DUMBBELL-TEST");

bool m_state_flag = false;
static void ChangeDataRate1 ()
{
  if (m_state_flag)
    {
      Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      Config::Set ("/NodeList/*/DeviceList/1/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      m_state_flag = false;
    }
  else
    {
	   Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("10Mbps"));
	   Config::Set ("/NodeList/*/DeviceList/1/$ns3::PointToPointNetDevice/DataRate", StringValue ("10Mbps"));
      m_state_flag = true;
    }
    Simulator::Schedule (Seconds (15), &ChangeDataRate1);
}

static void ChangeDataRate0 ()
{
  if (m_state_flag)
    {
      Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("0Mbps"));
      m_state_flag = false;
    }
  else
    {
	   Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("20Mbps"));
	   Config::Set ("/NodeList/*/DeviceList/0/$ns3::PointToPointNetDevice/DataRate", StringValue ("20Mbps"));
      m_state_flag = true;
    }
    Simulator::Schedule (Seconds (10), &ChangeDataRate0);
}

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("DUMBBELL-TEST", LOG_LEVEL_INFO);

  // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
 // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("OnOff", LOG_LEVEL_ALL);

  bool tracing = true;
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
  std::string scenario = "1";

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
  nodes.Create (6);

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  NodeContainer n0n1 = NodeContainer(nodes.Get(0),nodes.Get(1));
  NodeContainer n1n2 = NodeContainer(nodes.Get(1),nodes.Get(2));
  NodeContainer n1n3 = NodeContainer(nodes.Get(1),nodes.Get(3));
  NodeContainer n3n4 = NodeContainer(nodes.Get(3),nodes.Get(4));
  NodeContainer n3n5 = NodeContainer(nodes.Get(3),nodes.Get(5));

  NodeContainer n6n2 = NodeContainer(nodes.Get(1),nodes.Get(2));
 // NodeContainer n3n7 = NodeContainer(nodes.Get(3),nodes.Get(5));


  InternetStackHelper internet;
  internet.Install(nodes);

  PointToPointHelper p2prx;
  p2prx.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d0d1 =p2prx.Install(n0n1);

  PointToPointHelper r2r;
  r2r.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
  r2r.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d1d3 =r2r.Install(n1n3);

  PointToPointHelper p2ptx1;
  p2ptx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2ptx1.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d3d4 =p2ptx1.Install(n3n4);
  //tchPfifoFastAccess.Install(d3d4);

  PointToPointHelper p2ptx2;
  p2ptx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2ptx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d3d5 =p2ptx2.Install(n3n5);
  //tchPfifoFastAccess.Install(d3d5);

  PointToPointHelper p2prx1;
  p2prx1.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx1.SetChannelAttribute("Delay",StringValue("1ms"));
 // p2p.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));
  NetDeviceContainer d1d2 =p2prx1.Install(n1n2);

  PointToPointHelper p2prx2;
  p2prx2.SetDeviceAttribute("DataRate",StringValue("20Mbps"));
  p2prx2.SetChannelAttribute("Delay",StringValue("1ms"));
  NetDeviceContainer d6d2 =p2prx2.Install(n6n2);
  //NetDeviceContainer d3d7 =p2p.Install(n3n7);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0","255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign(d0d1);

  ipv4.SetBase("10.1.2.0","255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(d1d2);

  ipv4.SetBase("10.1.6.0","255.255.255.0");
  ipv4.Assign(d6d2);

  ipv4.SetBase("10.1.3.0","255.255.255.0");
  ipv4.Assign(d1d3);

  ipv4.SetBase("10.1.4.0","255.255.255.0");
  Ipv4InterfaceContainer i3i4 = ipv4.Assign(d3d4);

  ipv4.SetBase("10.1.5.0","255.255.255.0");
  ipv4.Assign(d3d5);
  Ipv4InterfaceContainer i3i5 = ipv4.Assign(d3d5);

 // ipv4.SetBase("10.1.7.0","255.255.255.0");
 // ipv4.Assign(d3d7);

  uint16_t port = 50000;
  ApplicationContainer sinkApp, sinkApp1;
  ApplicationContainer clientApps, clientApps2;

  //server
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  sinkApp.Add(sinkHelper.Install(nodes.Get(2)));
  sinkApp.Add(sinkHelper.Install(nodes.Get(0)));
  sinkApp.Start (Seconds (start_time));
  sinkApp.Stop (Seconds (stop_time + 1));

  //client 1
  BulkSendHelper source ("ns3::TcpSocketFactory",Address ());//InetSocketAddress (i1i2.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  source.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));

  AddressValue remoteAddress (InetSocketAddress (i1i2.GetAddress (1), port));
  source.SetAttribute ("Remote", remoteAddress);
  clientApps = source.Install (nodes.Get(5));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time + 1));

  AddressValue remoteAddress1 (InetSocketAddress (i0i1.GetAddress (0), port));
  source.SetAttribute ("Remote", remoteAddress1);
  clientApps = source.Install (nodes.Get(4));
  clientApps.Start(Seconds(start_time));
  clientApps.Stop (Seconds (stop_time - 10));

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

  std::string dir = "results/" + transport_prot.substr(5, transport_prot.length()) + "/";
  std::string dirToSave = "mkdir -p " + dir;
  if (tracing)
    {
 //     AsciiTraceHelper ascii;
  //    p2prx1.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      p2prx.EnablePcapAll ("tcp-bulk-send", false);
    }

  if (scenario == "2")
    {
        Simulator::Schedule (Seconds (5), &ChangeDataRate0);
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
