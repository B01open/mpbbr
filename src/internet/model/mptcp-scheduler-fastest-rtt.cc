/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
 *
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
 *
 * Author:  Kashif Nadeem <kshfnadeem@gmail.com>
 *          Matthieu Coudron <matthieu.coudron@lip6.fr>
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */
#include "ns3/mptcp-scheduler-fastest-rtt.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/log.h"

#include "ns3/output-stream-wrapper.h"
#include "ns3/trace-helper.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE("MpTcpSchedulerFastestRTT");

namespace ns3
{

Ptr<OutputStreamWrapper> sfrtt1;
Ptr<OutputStreamWrapper> sfrtt2;
Ptr<OutputStreamWrapper> sfrtt3;
Ptr<OutputStreamWrapper> sfrtt4;
Ptr<OutputStreamWrapper> sfrtt5;
Ptr<OutputStreamWrapper> sfrtt6;
Ptr<OutputStreamWrapper> sfrtt7;
Ptr<OutputStreamWrapper> sfrtt8;

std::string fastrttdir = "results-fast-rtt/";
std::string fastrttdirToSave = "mkdir -p " + fastrttdir;

static inline
SequenceNumber64 SEQ32TO64(SequenceNumber32 seq)
{
  return SequenceNumber64( seq.GetValue());
}

TypeId
MpTcpSchedulerFastestRTT::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpSchedulerFastestRTT")
    .SetParent<MpTcpScheduler> ()
    .AddConstructor<MpTcpSchedulerFastestRTT> ()
  ;

  return tid;
}

MpTcpSchedulerFastestRTT::MpTcpSchedulerFastestRTT()
  : MpTcpScheduler(),
    m_metaSock(0)
{
  NS_LOG_FUNCTION(this);

  system (fastrttdirToSave.c_str ());

  AsciiTraceHelper asciisfRTT1;
  std::string sfrttTrFileName1 = fastrttdir + "sf-rtt1";
  sfrtt1 = asciisfRTT1.CreateFileStream (sfrttTrFileName1.c_str ());

  AsciiTraceHelper asciisfRTT2;
  std::string sfrttTrFileName2 = fastrttdir + "sf-rtt2";
  sfrtt2 = asciisfRTT2.CreateFileStream (sfrttTrFileName2.c_str ());

  AsciiTraceHelper asciisfRTT3;
  std::string sfrttTrFileName3 = fastrttdir + "sf-rtt3";
  sfrtt3 = asciisfRTT3.CreateFileStream (sfrttTrFileName3.c_str ());

  AsciiTraceHelper asciisfRTT4;
  std::string sfrttTrFileName4 = fastrttdir + "sf-rtt4";
  sfrtt4 = asciisfRTT4.CreateFileStream (sfrttTrFileName4.c_str ());

  AsciiTraceHelper asciisfRTT5;
  std::string sfrttTrFileName5 = fastrttdir + "sf-rtt5";
  sfrtt5 = asciisfRTT5.CreateFileStream (sfrttTrFileName5.c_str ());

  AsciiTraceHelper asciisfRTT6;
  std::string sfrttTrFileName6 = fastrttdir + "sf-rtt6";
  sfrtt6 = asciisfRTT6.CreateFileStream (sfrttTrFileName6.c_str ());

  AsciiTraceHelper asciisfRTT7;
  std::string sfrttTrFileName7 = fastrttdir + "sf-rtt7";
  sfrtt7 = asciisfRTT7.CreateFileStream (sfrttTrFileName7.c_str ());

  AsciiTraceHelper asciisfRTT8;
  std::string sfrttTrFileName8 = fastrttdir + "sf-rtt8";
  sfrtt8 = asciisfRTT8.CreateFileStream (sfrttTrFileName8.c_str ());

}

MpTcpSchedulerFastestRTT::~MpTcpSchedulerFastestRTT (void)
{
  NS_LOG_FUNCTION(this);
}

void
MpTcpSchedulerFastestRTT::SetMeta(Ptr<MpTcpSocketBase> metaSock)
{
  NS_ASSERT(metaSock);
  NS_ASSERT_MSG(m_metaSock == 0, "SetMeta already called");
  m_metaSock = metaSock;
}

int
MpTcpSchedulerFastestRTT::FindFastestSubflowWithFreeWindow() const
{
  NS_LOG_FUNCTION_NOARGS();
  //yqy
  for (int i = 0; i < (int) m_metaSock->GetNActiveSubflows() ; i++ )
    {
      Ptr<MpTcpSubflow> sf = m_metaSock->GetSubflow(i);

	  if(i == 0){
		   	  *sfrtt1->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds()<< std::endl;
      }
      if(i == 1){
   	   	     *sfrtt2->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 2){
             *sfrtt3->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 3){
             *sfrtt4->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 4){
             *sfrtt5->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 5){
             *sfrtt6->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 6){
             *sfrtt7->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }
      if(i == 7){
             *sfrtt8->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->Window() << "   "<< sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
      }

   }



  int id = -1;
  Time lowestEstimate = Time::Max();
  for (int i = 0; i < (int) m_metaSock->GetNActiveSubflows() ; i++ )
    {
      Ptr<MpTcpSubflow> sf = m_metaSock->GetSubflow(i);
      if(sf->AvailableWindow() <= 0)
      {
        continue;
      }
      if(sf->m_rtt->GetEstimate () < lowestEstimate)
      {
        lowestEstimate = sf->m_rtt->GetEstimate ();
        id = i;
      }
   }
  return id;
}

Ptr<MpTcpSubflow>
MpTcpSchedulerFastestRTT::GetSubflowToUseForEmptyPacket()
{
  NS_ASSERT(m_metaSock->GetNActiveSubflows() > 0 );
  return  m_metaSock->GetSubflow(0);
}

bool
MpTcpSchedulerFastestRTT::GenerateMapping(int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_metaSock);

  uint32_t amountOfDataToSend = 0;
  //! Tx data not sent to subflows yet
  SequenceNumber32 metaNextTxSeq = m_metaSock->Getvalue_nextTxSeq();
  amountOfDataToSend = m_metaSock->m_txBuffer->SizeFromSequence( metaNextTxSeq );
  uint32_t metaWindow = m_metaSock->AvailableWindow();
  if (amountOfDataToSend <= 0)
    {
      NS_LOG_DEBUG("Nothing to send from meta");
      return false;
    }
  if (metaWindow <= 0)
   {
     return false;
    }

  int id = FindFastestSubflowWithFreeWindow();
  if(id < 0)
    {
      NS_LOG_DEBUG("No valid subflow");
      return false;
    }
  Ptr<MpTcpSubflow> subflow = m_metaSock->GetSubflow(id);
  uint32_t subflowWindow = subflow->AvailableWindow();
  uint32_t canSend = std::min( subflowWindow, metaWindow);

  //! Can't send more than SegSize
  //metaWindow en fait on s'en fout du SegSize ?
  if (canSend > 0)
    {
	  std::cout << "MpTcpSchedulerFastestRTT , activeSubflowArrayId = " << id << std::endl;
	   activeSubflowArrayId = id;
      dsn = SEQ32TO64(metaNextTxSeq);
      canSend = std::min(canSend, amountOfDataToSend);
      // For now we limit ourselves to a per packet basis
      length = std::min(canSend, subflow->GetSegSize());
      return true;
    }
  return false;
}

} // namespace ns3
