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

#include "ns3/mptcp-scheduler-round-robin.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/log.h"

#include "ns3/output-stream-wrapper.h"
#include "ns3/trace-helper.h"


NS_LOG_COMPONENT_DEFINE("MpTcpSchedulerRoundRobin");

namespace ns3
{

Ptr<OutputStreamWrapper> rrsfrtt1;
Ptr<OutputStreamWrapper> rrsfrtt2;
Ptr<OutputStreamWrapper> rrsfrtt3;
Ptr<OutputStreamWrapper> rrsfrtt4;
Ptr<OutputStreamWrapper> rrsfrtt5;
Ptr<OutputStreamWrapper> rrsfrtt6;
Ptr<OutputStreamWrapper> rrsfrtt7;
Ptr<OutputStreamWrapper> rrsfrtt8;
Ptr<OutputStreamWrapper> rrsfrtt9;

std::string rttdir = "results-sfrtt/";
std::string rttdirToSave = "mkdir -p " + rttdir;

int MpTcpSchedulerRoundRobin::iSfId = 0;

static inline
SequenceNumber64 SEQ32TO64(SequenceNumber32 seq)
{
  return SequenceNumber64( seq.GetValue());
}

TypeId
MpTcpSchedulerRoundRobin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpSchedulerRoundRobin")
    .SetParent<MpTcpScheduler> ()
    .AddConstructor<MpTcpSchedulerRoundRobin> ()
  ;
  return tid;
}

MpTcpSchedulerRoundRobin::MpTcpSchedulerRoundRobin() :
  MpTcpScheduler(),
  m_lastUsedFlowId(0),
  m_metaSock(0)
{
  NS_LOG_FUNCTION(this);

  system (rttdirToSave.c_str ());

  AsciiTraceHelper asciisfRTT1;
  std::string sfrttTrFileName1 = rttdir + "sf-rtt1";
  rrsfrtt1 = asciisfRTT1.CreateFileStream (sfrttTrFileName1.c_str ());

  AsciiTraceHelper asciisfRTT2;
  std::string sfrttTrFileName2 = rttdir + "sf-rtt2";
  rrsfrtt2 = asciisfRTT2.CreateFileStream (sfrttTrFileName2.c_str ());

  AsciiTraceHelper asciisfRTT3;
  std::string sfrttTrFileName3 = rttdir + "sf-rtt3";
  rrsfrtt3 = asciisfRTT3.CreateFileStream (sfrttTrFileName3.c_str ());

  AsciiTraceHelper asciisfRTT4;
  std::string sfrttTrFileName4 = rttdir + "sf-rtt4";
  rrsfrtt4 = asciisfRTT4.CreateFileStream (sfrttTrFileName4.c_str ());

  AsciiTraceHelper asciisfRTT5;
  std::string sfrttTrFileName5 = rttdir + "sf-rtt5";
  rrsfrtt5 = asciisfRTT5.CreateFileStream (sfrttTrFileName5.c_str ());

  AsciiTraceHelper asciisfRTT6;
  std::string sfrttTrFileName6 = rttdir + "sf-rtt6";
  rrsfrtt6 = asciisfRTT6.CreateFileStream (sfrttTrFileName6.c_str ());

  AsciiTraceHelper asciisfRTT7;
  std::string sfrttTrFileName7 = rttdir + "sf-rtt7";
  rrsfrtt7 = asciisfRTT7.CreateFileStream (sfrttTrFileName7.c_str ());

  AsciiTraceHelper asciisfRTT8;
  std::string sfrttTrFileName8 = rttdir + "sf-rtt8";
  rrsfrtt8 = asciisfRTT8.CreateFileStream (sfrttTrFileName8.c_str ());

  AsciiTraceHelper asciisfRTT9;
  std::string sfrttTrFileName9 = rttdir + "sf-rtt9";
  rrsfrtt9 = asciisfRTT9.CreateFileStream (sfrttTrFileName9.c_str ());

}

MpTcpSchedulerRoundRobin::~MpTcpSchedulerRoundRobin (void)
{
  NS_LOG_FUNCTION(this);
}

void
MpTcpSchedulerRoundRobin::SetMeta(Ptr<MpTcpSocketBase> metaSock)
{
  NS_ASSERT(metaSock);
  NS_ASSERT_MSG(m_metaSock == 0, "SetMeta already called");
  m_metaSock = metaSock;
}

int
MpTcpSchedulerRoundRobin::AverageSubfRttWithFreeWindow() const
{
  NS_LOG_FUNCTION_NOARGS();
  Time sumTime = Seconds (0);
  Time aveTime = Seconds (0);
  int iCountNum = 0;

  Time lowestEstimate = Time::Max();
  for (int i = 0; i < (int) m_metaSock->GetNActiveSubflows() ; i++ )
  {
      Ptr<MpTcpSubflow> sf = m_metaSock->GetSubflow(i);
      if(sf->AvailableWindow() <= 0)
      {
        continue;
      }
      iCountNum++;
      sumTime += sf->m_rtt->GetEstimate ();
  }

  aveTime = sumTime / iCountNum;
  NS_LOG_ERROR("simulate time = " << Simulator::Now ().GetSeconds () <<", aveRtt = " << aveTime.GetSeconds() );

  for(int i = 0; i < (int) m_metaSock->GetNActiveSubflows() ; i++ )
  {
	  Ptr<MpTcpSubflow> sf = m_metaSock->GetSubflow(i);
	  if(sf->AvailableWindow() <= 0)
	  {
	    continue;
	  }

	  NS_LOG_ERROR("no ave ; subflow i " << i << "sf-> rtt = " << sf->m_rtt->GetEstimate().GetSeconds());

	  sf->m_rtt->GetEstimate () = sf->m_rtt->GetEstimate () +  0.001 * (aveTime - sf->m_rtt->GetEstimate ());

	  NS_LOG_ERROR("subflow i " << i << "sf-> rtt = " << sf->m_rtt->GetEstimate().GetSeconds());
  }

  return 0;
}

int
MpTcpSchedulerRoundRobin::recordSubflowRTT() const
{
	 for(int i = 0; i < (int) m_metaSock->GetNActiveSubflows() ; i++ )
	  {
		  Ptr<MpTcpSubflow> sf = m_metaSock->GetSubflow(i);

		  	  	if(i == 0){
		  		   	   *rrsfrtt1->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 1){
		     	   	   *rrsfrtt2->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 2){
		               *rrsfrtt3->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 3){
		               *rrsfrtt4->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 4){
		               *rrsfrtt5->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 5){
		               *rrsfrtt6->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 6){
		               *rrsfrtt7->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 7){
		               *rrsfrtt8->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }
		        if(i == 8){
		               *rrsfrtt9->GetStream () << Simulator::Now ().GetSeconds () << " " << sf->m_rtt->GetEstimate().GetSeconds() << std::endl;
		        }

	  }
  return 0;
}

Ptr<MpTcpSubflow>
MpTcpSchedulerRoundRobin::GetSubflowToUseForEmptyPacket()
{
  NS_ASSERT(m_metaSock->GetNActiveSubflows() > 0 );
  return  m_metaSock->GetSubflow(0);
}

/* We assume scheduler can't send data on subflows, so it can just generate mappings */
bool
MpTcpSchedulerRoundRobin::GenerateMapping(int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_metaSock);

  int nbOfSubflows = m_metaSock->GetNActiveSubflows();
  int attempt = 0;
  uint32_t amountOfDataToSend = 0;

  //! Tx data not sent to subflows yet
  SequenceNumber32 metaNextTxSeq = m_metaSock->Getvalue_nextTxSeq(); 
  amountOfDataToSend = m_metaSock->m_txBuffer->SizeFromSequence( metaNextTxSeq );
  uint32_t metaWindow = m_metaSock->AvailableWindow();

  if(amountOfDataToSend <= 0)
   {
     NS_LOG_DEBUG("Nothing to send from meta");
     return false;
   }

  if(metaWindow <= 0)
   {
     NS_LOG_DEBUG("No meta window available (TODO should be in persist state ?)");
     return false;
   }

 // AverageSubfRttWithFreeWindow();
  recordSubflowRTT();

  while(attempt < nbOfSubflows)
     {
       attempt++;
       m_lastUsedFlowId = (m_lastUsedFlowId + 1) % nbOfSubflows;

       NS_LOG_ERROR("m_lastUsedFlowId = " << (int)(m_lastUsedFlowId));

       iSfId = (int)(m_lastUsedFlowId);

       Ptr<MpTcpSubflow> subflow = m_metaSock->GetSubflow(m_lastUsedFlowId);
       uint32_t subflowWindow = subflow->AvailableWindow();

       std::cout << "m_lastUsedFlowId = " << (int)(m_lastUsedFlowId) <<std::endl;
       std::cout<<"subflowWindow = "<<subflowWindow << "  metaWindow = " <<metaWindow <<std::endl;
       uint32_t canSend = std::min( subflowWindow, metaWindow);

       //! Can't send more than SegSize
       //metaWindow en fait on s'en fout du SegSize ?
       if(canSend > 0)
        {
          activeSubflowArrayId = m_lastUsedFlowId;
          dsn = SEQ32TO64(metaNextTxSeq);
          canSend = std::min(canSend, amountOfDataToSend);
          // For now we limit ourselves to a per packet basis
          length = std::min(canSend, subflow->GetSegSize());
          return true;
        }
     }
  return false;
}

} // end of 'ns3'
