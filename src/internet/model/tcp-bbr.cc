/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
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
 * Authors: Vivek Jain <jain.vivek.anand@gmail.com>
 *          Viyom Mittal <viyommittal@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "tcp-bbr.h"
#include "ns3/log.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/trace-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpBbr");
NS_OBJECT_ENSURE_REGISTERED (TcpBbr);

const double TcpBbr::PACING_GAIN_CYCLE [] = {5.0 / 4, 3.0 / 4, 1, 1, 1, 1, 1, 1};

Ptr<OutputStreamWrapper> inflightbbr;
Ptr<OutputStreamWrapper> pacingbbr;
Ptr<OutputStreamWrapper> pacingbbr2;
Ptr<OutputStreamWrapper> pacingbbr3;
Ptr<OutputStreamWrapper> pacingbbr4;
Ptr<OutputStreamWrapper> rttbbr;
Ptr<OutputStreamWrapper> cwndbbr;
Ptr<OutputStreamWrapper> cwndbbr2;
Ptr<OutputStreamWrapper> cwndbbr3;
Ptr<OutputStreamWrapper> cwndbbr4;
Ptr<OutputStreamWrapper> logbbr;

std::string bbrdir = "results-bbr/";
std::string bbrdirToSave = "mkdir -p " + bbrdir;


TypeId
TcpBbr::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpBbr")
    .SetParent<TcpCongestionOps> ()
    .AddConstructor<TcpBbr> ()
    .SetGroupName ("Internet")
    .AddAttribute ("HighGain",
                   "Value of high gain",
                   DoubleValue (2.885),
                   MakeDoubleAccessor (&TcpBbr::m_highGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BwWindowLength",
                   "Length of bandwidth windowed filter",
                   UintegerValue (10),
                   MakeUintegerAccessor (&TcpBbr::m_bandwidthWindowLength),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RttWindowLength",
                   "Length of bandwidth windowed filter",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&TcpBbr::m_rtPropFilterLen),
                   MakeTimeChecker ())
    .AddAttribute ("ProbeRttDuration",
                   "Length of bandwidth windowed filter",
                   TimeValue (MilliSeconds (200)),
                   MakeTimeAccessor (&TcpBbr::m_probeRttDuration),
                   MakeTimeChecker ())
  ;
  return tid;
}

TcpBbr::TcpBbr ()
  : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

TcpBbr::TcpBbr (const TcpBbr &sock)
  : TcpCongestionOps (sock),
    m_bandwidthWindowLength (sock.m_bandwidthWindowLength),
    m_pacingGain (sock.m_pacingGain),
    m_cWndGain (sock.m_cWndGain),
    m_highGain (sock.m_highGain),
    m_isPipeFilled (sock.m_isPipeFilled),
    m_minPipeCwnd (sock.m_minPipeCwnd),
    m_roundCount (sock.m_roundCount),
    m_roundStart (sock.m_roundStart),
    m_nextRoundDelivered (sock.m_nextRoundDelivered),
    m_probeRttDuration (sock.m_probeRttDuration),
    m_probeRtPropStamp (sock.m_probeRtPropStamp),
    m_probeRttDoneStamp (sock.m_probeRttDoneStamp),
    m_probeRttRoundDone (sock.m_probeRttRoundDone),
    m_packetConservation (sock.m_packetConservation),
    m_priorCwnd (sock.m_priorCwnd),
    m_idleRestart (sock.m_idleRestart),
    m_targetCWnd (sock.m_targetCWnd),
    m_fullBandwidth (sock.m_fullBandwidth),
    m_fullBandwidthCount (sock.m_fullBandwidthCount),
    m_rtProp (Time::Max ()),
    m_sendQuantum (sock.m_sendQuantum),
    m_cycleStamp (sock.m_cycleStamp),
    m_cycleIndex (sock.m_cycleIndex),
    m_rtPropExpired (sock.m_rtPropExpired),
    m_rtPropFilterLen (sock.m_rtPropFilterLen),
    m_rtPropStamp (sock.m_rtPropStamp),
    m_isInitialized (sock.m_isInitialized)
{
  NS_LOG_FUNCTION (this);

    system (bbrdirToSave.c_str ());

	AsciiTraceHelper asciiInflight;
	std::string inflightTrFileName = bbrdir + "bbr-inflight";
	inflightbbr = asciiInflight.CreateFileStream (inflightTrFileName.c_str ());

	AsciiTraceHelper asciiPacing;
	std::string pacingTrFileName = bbrdir + "bbr-pacing";
	pacingbbr = asciiPacing.CreateFileStream (pacingTrFileName.c_str ());

	AsciiTraceHelper asciiPacing2;
	std::string pacingTrFileName2 = bbrdir + "bbr-pacing2";
	pacingbbr2 = asciiPacing2.CreateFileStream (pacingTrFileName2.c_str ());

	AsciiTraceHelper asciiPacing3;
	std::string pacingTrFileName3 = bbrdir + "bbr-pacing3";
	pacingbbr3 = asciiPacing3.CreateFileStream (pacingTrFileName3.c_str ());

	AsciiTraceHelper asciiPacing4;
	std::string pacingTrFileName4 = bbrdir + "bbr-pacing4";
	pacingbbr4 = asciiPacing4.CreateFileStream (pacingTrFileName4.c_str ());

	AsciiTraceHelper asciiRTT;
	std::string rttTrFileName = bbrdir +  "bbr-rtt";
	rttbbr = asciiRTT.CreateFileStream (rttTrFileName.c_str ());

	AsciiTraceHelper ascii;
	std::string cwndTrFileName = bbrdir + "bbr-cwnd";
	cwndbbr = ascii.CreateFileStream (cwndTrFileName.c_str ());

	AsciiTraceHelper ascii2;
	std::string cwndTrFileName2 = bbrdir + "bbr-cwnd2";
	cwndbbr2 = ascii2.CreateFileStream (cwndTrFileName2.c_str ());

	AsciiTraceHelper ascii3;
	std::string cwndTrFileName3 = bbrdir + "bbr-cwnd3";
	cwndbbr3 = ascii3.CreateFileStream (cwndTrFileName3.c_str ());

	AsciiTraceHelper ascii4;
	std::string cwndTrFileName4 = bbrdir + "bbr-cwnd4";
	cwndbbr4 = ascii4.CreateFileStream (cwndTrFileName4.c_str ());

	AsciiTraceHelper asciiLog;
	std::string logName = bbrdir + "bbr-log";
	logbbr = asciiLog.CreateFileStream (logName.c_str ());
	*logbbr->GetStream () << "bbr log info :" << std::endl;

  m_uv = CreateObject<UniformRandomVariable> ();
}

int64_t
TcpBbr::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "AssignStreams" << std::endl;
  m_uv->SetStream (stream);
  return 1;
}

void
TcpBbr::InitRoundCounting ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "InitRoundCounting" << std::endl;
  m_nextRoundDelivered = 0;
  m_roundStart = false;
  m_roundCount = 0;
}

void
TcpBbr::InitFullPipe ()
{
  NS_LOG_FUNCTION (this);
  m_isPipeFilled = false;
  m_fullBandwidth = 0;
  m_fullBandwidthCount = 0;
}

void
TcpBbr::InitPacingRate (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "InitPacingRate" << std::endl;
  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << "   " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;

  if (!tcb->m_pacing)
    {
      NS_LOG_WARN ("BBR must use pacing");
      tcb->m_pacing = true;
    }
  Time rtt = tcb->m_lastRtt_bbr != Time::Max () ? tcb->m_lastRtt_bbr : MilliSeconds (1);
  DataRate nominalBandwidth (tcb->m_initialCWnd * tcb->m_segmentSize * 8 / rtt.GetSeconds ());
  tcb->m_currentPacingRate = DataRate (m_pacingGain * nominalBandwidth.GetBitRate ());
}

void
TcpBbr::EnterStartup ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "WhichState " << WhichState (m_state) << " m_rtPropExpired " << m_rtPropExpired << " !m_idleRestart " << !m_idleRestart);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "EnterStartup" << std::endl;
  SetBbrState (BbrMode_t::BBR_STARTUP);
 // m_pacingGain = m_highGain + 1;
 // m_cWndGain = m_highGain - 0.885;
  m_pacingGain = m_highGain;
  m_cWndGain = m_highGain;

}

void
TcpBbr::HandleRestartFromIdle (Ptr<TcpSocketState> tcb, const RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "HandleRestartFromIdle" << std::endl;

  if (tcb->m_bytesInFlight_bbr == 0 && rs->m_isAppLimited)
    {
      m_idleRestart = true;
      if (m_state == BbrMode_t::BBR_PROBE_BW)
        {
          SetPacingRate (tcb, 1);
        }
    }
}

void
TcpBbr::SetPacingRate (Ptr<TcpSocketState> tcb, double gain)
{
  NS_LOG_FUNCTION (this << tcb << gain);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "SetPacingRate" << std::endl;
  DataRate rate (gain * m_maxBwFilter.GetBest ().GetBitRate ());
  rate = std::min (rate, tcb->m_maxPacingRate);
  if (m_isPipeFilled || rate > tcb->m_currentPacingRate)
    {
      tcb->m_currentPacingRate = rate;
    }
}

uint32_t
TcpBbr::InFlight (Ptr<TcpSocketState> tcb, double gain)
{
  NS_LOG_FUNCTION (this << tcb << gain);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "InFlight" << std::endl;
  if (m_rtProp == Time::Max ())
    {
      return tcb->m_initialCWnd * tcb->m_segmentSize;
    }
  double quanta = 3 * m_sendQuantum;
  double estimatedBdp = m_maxBwFilter.GetBest () * m_rtProp / 8.0;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << (gain * estimatedBdp + quanta)/1.0/tcb->m_segmentSize << std::endl;
  return gain * estimatedBdp + quanta;
}

void
TcpBbr::AdvanceCyclePhase ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "AdvanceCyclePhase" << std::endl;
  m_cycleStamp = Simulator::Now ();
  m_cycleIndex = (m_cycleIndex + 1) % GAIN_CYCLE_LENGTH;
  m_pacingGain = PACING_GAIN_CYCLE [m_cycleIndex];
}

bool
TcpBbr::IsNextCyclePhase (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "IsNextCyclePhase" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;

  bool isFullLength = (Simulator::Now () - m_cycleStamp) > m_rtProp; //std::min(m_rtProp, MilliSeconds (5));  //m_rtProp, 每一轮探测的时长
  if (m_pacingGain == 1)
    {
      return isFullLength;
    }
  else if (m_pacingGain > 1)
    {
      return isFullLength && (rs->m_packetLoss > 0 || tcb->m_priorInFlight >= InFlight (tcb, m_pacingGain));   //bdp  * 1.25
    }
  else
    {
      return isFullLength || tcb->m_priorInFlight <= InFlight (tcb, 1);   //drain  0.75 * bdp
    }
}

void
TcpBbr::CheckCyclePhase (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "CheckCyclePhase" << std::endl;
  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd.Get ()/1.0 /tcb->m_segmentSize << std::endl;
//  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  //在探测带宽阶段，判断是否进入到探测带宽阶段的下一阶段
  if (m_state == BbrMode_t::BBR_PROBE_BW && IsNextCyclePhase (tcb, rs))
    {
      AdvanceCyclePhase ();
    }
}

void
TcpBbr::CheckFullPipe (const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "CheckFullPipe" << std::endl;

  if (m_isPipeFilled || !m_roundStart || rs->m_isAppLimited)
    {
      return;
    }

  /* Check if Bottleneck bandwidth is still growing*/
  if (m_maxBwFilter.GetBest ().GetBitRate () >= m_fullBandwidth.GetBitRate () * 1.25)   //1.25,  此处满带宽乘以的参数需要和增益保持一致
    {
      m_fullBandwidth = m_maxBwFilter.GetBest ();
      m_fullBandwidthCount = 0;
      return;
    }

  m_fullBandwidthCount++;
  if (m_fullBandwidthCount >= 3)
    {
      NS_LOG_DEBUG ("Pipe filled");
      m_isPipeFilled = true;
    }
}

void
TcpBbr::EnterDrain ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  "EnterDrain" << std::endl;
  SetBbrState (BbrMode_t::BBR_DRAIN);
  m_pacingGain = 1.0 / m_highGain;
  m_cWndGain = m_highGain;
}

void
TcpBbr::EnterProbeBW ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "EnterProbeBW" << std::endl;
  SetBbrState (BbrMode_t::BBR_PROBE_BW);
  m_pacingGain = 1;
  m_cWndGain = 2;
  m_cycleIndex = GAIN_CYCLE_LENGTH - 1 - (int) m_uv->GetValue (0, 8);    //GAIN_CYCLE_LENGTH, 当修改探测带宽周期长度时，需要修改随机变量取值范围
  AdvanceCyclePhase ();
}

void
TcpBbr::CheckDrain (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "CheckDrain" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;
  //*inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if (m_state == BbrMode_t::BBR_STARTUP && m_isPipeFilled)
    {
      EnterDrain ();
    }

  if (m_state == BbrMode_t::BBR_DRAIN && tcb->m_bytesInFlight_bbr <= InFlight (tcb, 1))     //排空到底需要排空到一个什么程度
    {
      EnterProbeBW ();
    }
}

void
TcpBbr::UpdateRTprop (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "UpdateRTprop" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;

  m_rtPropExpired = Simulator::Now () > (m_rtPropStamp + m_rtPropFilterLen);
  if (tcb->m_lastRtt_bbr >= Seconds (0) && (tcb->m_lastRtt_bbr <= m_rtProp || m_rtPropExpired))
    {
      m_rtProp = tcb->m_lastRtt_bbr;
      m_rtPropStamp = Simulator::Now ();

     // *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
    }
}

void
TcpBbr::EnterProbeRTT ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "EnterProbeRTT" << std::endl;
  NS_LOG_DEBUG("##########################EnterProbeRTT");

  SetBbrState (BbrMode_t::BBR_PROBE_RTT);
  m_pacingGain = 1;
  m_cWndGain = 1;
}

void
TcpBbr::SaveCwnd (Ptr<const TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "SaveCwnd" << std::endl;
  if (tcb->m_congState != TcpSocketState::CA_RECOVERY && m_state != BbrMode_t::BBR_PROBE_RTT)
    {
      m_priorCwnd = tcb->m_cWnd;
    }
  else
    {
      m_priorCwnd = std::max (m_priorCwnd, tcb->m_cWnd.Get ());
    }
//  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;
}

void
TcpBbr::RestoreCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "RestoreCwnd" << std::endl;
  tcb->m_cWnd = std::max (m_priorCwnd, tcb->m_cWnd.Get ());
  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () <<  "     "<< tcb->m_cWnd.Get ()/1.0 /tcb->m_segmentSize << std::endl;
  //*inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
}

void
TcpBbr::ExitProbeRTT ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "ExitProbeRTT" << std::endl;
  if (m_isPipeFilled)
    {
      EnterProbeBW ();
    }
  else
    {
      EnterStartup ();
    }
}

void
TcpBbr::HandleProbeRTT (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "HandleProbeRTT" << std::endl;
 // *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0 << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  //按照变量定义的意思进行代码修改;  m_appLimited:  最后一个传输包的索引被标记为应用程序受限
  tcb->m_appLimited = (tcb->m_delivered + tcb->m_bytesInFlight_bbr) ? : 1;

  std::cout<<" tcb->m_appLimited = "<<tcb->m_appLimited<<std::endl;

  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "m_bytesInFlight_bbr =  " << tcb->m_bytesInFlight_bbr<< "  m_minPipeCwnd " << m_minPipeCwnd << std::endl;
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "m_probeRttDoneStamp =  " << m_probeRttDoneStamp.GetSeconds()<< "  " << Seconds (0) << std::endl;
  //bool testBool = false;

  if (m_probeRttDoneStamp == Seconds (0) && tcb->m_bytesInFlight_bbr <= m_minPipeCwnd)
    {
      m_probeRttDoneStamp = Simulator::Now () + m_probeRttDuration;
      m_probeRttRoundDone = false;
      m_nextRoundDelivered = tcb->m_delivered;
    }
  else if (m_probeRttDoneStamp != Seconds (0))
    {
	  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  m_roundStart =  " << m_roundStart<< std::endl;
      if (m_roundStart)
        {
          m_probeRttRoundDone = true;
        }

      *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  m_probeRttDoneStamp =  " << m_probeRttDoneStamp<< std::endl;
      if (m_probeRttRoundDone && Simulator::Now () > m_probeRttDoneStamp)
        {
          m_rtPropStamp = Simulator::Now ();
          RestoreCwnd (tcb);
          ExitProbeRTT ();
        }
    }
}

void
TcpBbr::CheckProbeRTT (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "CheckProbeRTT" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd << std::endl;
//  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;

  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "WhichState " << WhichState (m_state) << " m_rtPropExpired " << m_rtPropExpired << " !m_idleRestart " << !m_idleRestart);
  if (m_state != BbrMode_t::BBR_PROBE_RTT && m_rtPropExpired && !m_idleRestart)
    {
      EnterProbeRTT ();
      SaveCwnd (tcb);
      m_probeRttDoneStamp = Seconds (0);
      *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "m_state != BbrMode_t::BBR_PROBE_RTT" << std::endl;
    }

  if (m_state == BbrMode_t::BBR_PROBE_RTT)
    {
      HandleProbeRTT (tcb);
      *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "m_state == BbrMode_t::BBR_PROBE_RTT" << std::endl;
    }

  m_idleRestart = false;
}

void
TcpBbr::SetSendQuantum (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "SetSendQuantum" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
  m_sendQuantum = 1 * tcb->m_segmentSize;
/*TODO
  Since TSO can't be implemented in ns-3
  if (tcb->m_currentPacingRate < DataRate ("1.2Mbps"))
    {
      m_sendQuantum = 1 * tcb->m_segmentSize;
    }
  else if (tcb->m_currentPacingRate < DataRate ("24Mbps"))
    {
      m_sendQuantum  = 2 * tcb->m_segmentSize;
    }
  else
    {
      m_sendQuantum = std::min (tcb->m_currentPacingRate.GetBitRate () * MilliSeconds (1).GetMilliSeconds () / 8, (uint64_t) 64000);
    }*/
}

void
TcpBbr::UpdateTargetCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "UpdateTargetCwnd" << std::endl;
//  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << m_targetCWnd/1.0/tcb->m_segmentSize << std::endl;
//  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  m_targetCWnd = InFlight (tcb, m_cWndGain);

  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "   m_targetCWnd   " << m_targetCWnd << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << m_targetCWnd/1.0/tcb->m_segmentSize << std::endl;
}

void
TcpBbr::ModulateCwndForRecovery (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "ModulateCwndForRecovery" << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if ( rs->m_packetLoss > 0)
    {
	  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  rs->m_packetLoss =  " << rs->m_packetLoss  << std::endl;
	  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  tcb->m_cWnd.Get () =  " << tcb->m_cWnd.Get () << std::endl;

	  if(((int)(tcb->m_cWnd.Get ()) - (int)(rs->m_packetLoss)) < 0){
		  tcb->m_cWnd = tcb->m_segmentSize;
		  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  tcb->m_cWnd.Get () 1 =  " << tcb->m_cWnd.Get () << std::endl;
	  } else {
		  tcb->m_cWnd = std::max (tcb->m_cWnd.Get () - rs->m_packetLoss, tcb->m_segmentSize);
		  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  tcb->m_cWnd.Get ()2  =  " << tcb->m_cWnd.Get () << std::endl;
	  }
    }

  if (m_packetConservation)
    {
      tcb->m_cWnd = std::max (tcb->m_cWnd.Get (), tcb->m_bytesInFlight_bbr + tcb->m_lastAckedSackedBytes);
      //yqy
     // tcb->m_cWnd = std::max (tcb->m_cWnd.Get (), tcb->m_bytesInFlight_bbr + tcb->m_segmentSize);
	 // tcb->m_cWnd = std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), tcb-> m_cWnd.Get());
    }
}

void
TcpBbr::ModulateCwndForProbeRTT (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "ModulateCwndForProbeRTT" << std::endl;
 // *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if (m_state == BbrMode_t::BBR_PROBE_RTT)
    {
      tcb->m_cWnd = std::min (tcb->m_cWnd.Get (), m_minPipeCwnd);
      //yqy  single way
	 // tcb->m_cWnd = m_priorCwnd / 2.0;
    //  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
    }
}

void
TcpBbr::SetCwnd (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "SetCwnd" << std::endl;
  UpdateTargetCwnd (tcb);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "    UpdateTargetCwnd  " << tcb->m_cWnd / tcb->m_segmentSize<< std::endl;

  if (tcb->m_congState == TcpSocketState::CA_RECOVERY)
    {
      ModulateCwndForRecovery (tcb, rs);
      *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "ModulateCwndForRecovery  " << tcb->m_cWnd  / tcb->m_segmentSize << std::endl;
    }

  if (!m_packetConservation)
    {
      if (m_isPipeFilled)
        {
          tcb->m_cWnd = std::min (tcb->m_cWnd.Get () + (uint32_t) tcb->m_lastAckedSackedBytes, m_targetCWnd);
    	  //yqy
    	 // tcb->m_cWnd = std::min (tcb->m_cWnd.Get () + (uint32_t) tcb->m_segmentSize, m_targetCWnd);
        }
      else if (tcb->m_cWnd < m_targetCWnd || tcb->m_delivered < tcb->m_initialCWnd * tcb->m_segmentSize)
        {
          tcb->m_cWnd = tcb->m_cWnd.Get () + tcb->m_lastAckedSackedBytes;
    	  //yqy
    	//  tcb->m_cWnd = tcb->m_cWnd.Get () + tcb->m_segmentSize;
        }
      tcb->m_cWnd = std::max (tcb->m_cWnd.Get (), m_minPipeCwnd);

    //  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
    }
  ModulateCwndForProbeRTT (tcb);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "   ModulateCwndForProbeRTT  " << tcb->m_cWnd / tcb->m_segmentSize << std::endl;
  if (tcb->m_congState == TcpSocketState::CA_RECOVERY)
    {
      m_packetConservation = false;
    }
}

void
TcpBbr::UpdateRound (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "UpdateRound" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd /1.0/tcb->m_segmentSize << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if (tcb->m_txItemDelivered >= m_nextRoundDelivered)
    {
      m_nextRoundDelivered = tcb->m_delivered;
      m_roundCount++;
      m_roundStart = true;
    }
  else
    {
      m_roundStart = false;
    }
}

void
TcpBbr::UpdateBtlBw (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "UpdateBtlBw" << std::endl;

 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;

  if (rs->m_deliveryRate == 0)
    {
      return;
    }

  UpdateRound (tcb, rs);

  //当本轮发送速率大于上次最大发送速率，或者应用层不受限；则更新发送速率
  if (rs->m_deliveryRate >= m_maxBwFilter.GetBest () || !rs->m_isAppLimited)
    {
      m_maxBwFilter.Update (rs->m_deliveryRate, m_roundCount);
    }
}

void
TcpBbr::UpdateModelAndState (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "UpdateModelAndState" << std::endl;
  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd.Get () / 1.0 / tcb->m_segmentSize << std::endl;
  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  UpdateBtlBw (tcb, rs);    //更新带宽值
  CheckCyclePhase (tcb, rs);   //判断是否进入下一阶段
  CheckFullPipe (rs);
  CheckDrain (tcb);

  UpdateRTprop (tcb);
  CheckProbeRTT (tcb);
}

void
TcpBbr::UpdateControlParameters (Ptr<TcpSocketState> tcb, const struct RateSample * rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "UpdateControlParameters" << std::endl;
  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd.Get ()/1.0/tcb->m_segmentSize << std::endl;
  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  SetPacingRate (tcb, m_pacingGain);

  SetSendQuantum (tcb);

  SetCwnd (tcb, rs);

}

std::string
TcpBbr::WhichState (BbrMode_t mode) const
{
  switch (mode)
    {
    case 0:
      return "BBR_STARTUP";
    case 1:
      return "BBR_DRAIN";
    case 2:
      return "BBR_PROBE_BW";
    case 3:
      return "BBR_PROBE_RTT";
    }
  NS_ASSERT (false);
}

void
TcpBbr::SetBbrState (BbrMode_t mode)
{
  NS_LOG_FUNCTION (this << mode);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "SetBbrState" << std::endl;
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " Changing from " << WhichState (m_state) << " to " << WhichState (mode));
  m_state = mode;
}

uint32_t
TcpBbr::GetBbrState ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "GetBbrState" << std::endl;
  return m_state;
}

double
TcpBbr::GetCwndGain ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "GetCwndGain" << std::endl;
  return m_cWndGain;
}

double
TcpBbr::GetPacingGain ()
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "GetPacingGain" << std::endl;
  return m_pacingGain;
}

std::string
TcpBbr::GetName () const
{
  return "TcpBbr";
}

bool
TcpBbr::HasCongControl () const
{
  NS_LOG_FUNCTION (this);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "HasCongControl" << std::endl;
  return true;
}

void
TcpBbr::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                        const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "PktsAcked" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
 //   *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
 //   *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  CongControl (tcb, &tcb->m_rs);
}

void
TcpBbr::CongControl (Ptr<TcpSocketState> tcb, const struct RateSample *rs)
{
  NS_LOG_FUNCTION (this << tcb << rs);

  NS_LOG_ERROR("mptcpRoundRobin.m_lastUsedFlowId  = "  <<"  " << mptcpRoundRobin.iSfId);

  std::cout << Simulator::Now ().GetSeconds () << "     mptcpRoundRobin.iSfId = " <<  mptcpRoundRobin.iSfId << std::endl;

  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "CongControl" << std::endl;

 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () <<  "     " << tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;
  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << "   " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << "   " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  UpdateModelAndState (tcb, rs);

  UpdateControlParameters (tcb, rs);
}

void
TcpBbr::CongestionStateSet (Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
  *logbbr->GetStream () << Simulator::Now ().GetSeconds () << "  " << "CongestionStateSet" << std::endl;
 // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
  *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << "   " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
 // *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if (newState == TcpSocketState::CA_OPEN && !m_isInitialized)
    {
      NS_LOG_DEBUG ("CongestionStateSet triggered to CA_OPEN :: " << newState);
      m_rtProp = tcb->m_lastRtt_bbr != Time::Max () ? tcb->m_lastRtt_bbr : Time::Max ();
      m_rtPropStamp = Simulator::Now ();
      m_priorCwnd = tcb->m_initialCWnd * tcb->m_segmentSize;
      m_targetCWnd = tcb->m_initialCWnd * tcb->m_segmentSize;
      m_minPipeCwnd = 4 * tcb->m_segmentSize;
      m_sendQuantum = 1 * tcb->m_segmentSize;
//      m_maxBwFilter = MaxBandwidthFilter_t (m_bandwidthWindowLength,
//                                            DataRate (tcb->m_initialCWnd * tcb->m_segmentSize * 8 / m_rtProp.GetSeconds ())
//                                            , 0);
      m_maxBwFilter = MaxBandwidthFilter_t (m_bandwidthWindowLength, DataRate (0), 0);

   //   *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_initialCWnd << std::endl;

   //   *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;

      InitRoundCounting ();
      InitFullPipe ();
      EnterStartup ();
      InitPacingRate (tcb);
      m_isInitialized = true;
    }
  else if (newState == TcpSocketState::CA_LOSS)
    {
      NS_LOG_DEBUG ("CongestionStateSet triggered to CA_LOSS :: " << newState);
      SaveCwnd (tcb);
      m_roundStart = true;
    }
  else if (newState == TcpSocketState::CA_RECOVERY)
    {
      NS_LOG_DEBUG ("CongestionStateSet triggered to CA_RECOVERY :: " << newState);
      SaveCwnd (tcb);
      tcb->m_cWnd = tcb->m_bytesInFlight_bbr + std::max (tcb->m_lastAckedSackedBytes, tcb->m_segmentSize);
      //yqy not use sack
      //tcb->m_cWnd = tcb->m_bytesInFlight_bbr + tcb->m_segmentSize;
     // tcb->m_cWnd = std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), tcb-> m_cWnd.Get());

     // *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () <<  "     "<< tcb->m_cWnd/1.0 /tcb->m_segmentSize << std::endl;

      m_packetConservation = true;
    }
}

void
TcpBbr::CwndEvent (Ptr<TcpSocketState> tcb,
                   const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this << tcb << event);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "CwndEvent" << std::endl;
 // *rttbbr->GetStream () <<Simulator::Now ().GetSeconds () << " " <<  tcb->m_lastRtt_bbr.GetSeconds () << std::endl;
//  *cwndbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_cWnd/1.0/tcb->m_segmentSize << std::endl;
//  *inflightbbr->GetStream () << Simulator::Now ().GetSeconds () << " " << tcb->m_bytesInFlight_bbr/1.0/tcb->m_segmentSize << std::endl;
  if (event == TcpSocketState::CA_EVENT_COMPLETE_CWR)
    {
      NS_LOG_DEBUG ("CwndEvent triggered to CA_EVENT_COMPLETE_CWR :: " << event);
      m_packetConservation = false;
      RestoreCwnd (tcb);
    }
  else if (event == TcpSocketState::CA_EVENT_TX_START)
    {
      NS_LOG_DEBUG ("CwndEvent triggered to CA_EVENT_TX_START :: " << event);
      if (tcb->m_bytesInFlight_bbr == 0 && tcb->m_appLimited)
        {
          m_idleRestart = true;
          if (m_state == BbrMode_t::BBR_PROBE_BW && tcb->m_appLimited)
            {
              SetPacingRate (tcb, 1);
            }
        }
    }
}


uint32_t
TcpBbr::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  *logbbr->GetStream () <<Simulator::Now ().GetSeconds () << "  " <<  "GetSsThresh" << std::endl;
  SaveCwnd (tcb);

 // return tcb->m_initialSsThresh;

  return std::max (2 * tcb->m_segmentSize, bytesInFlight / 2);

}

Ptr<TcpCongestionOps>
TcpBbr::Fork (void)
{
  return CopyObject<TcpBbr> (this);
}

} // namespace ns3
