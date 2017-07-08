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

 #include <fstream>
 #include <iostream>
 #include <time.h>
 #include <sys/time.h>
 #include <stdlib.h>
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-layout-module.h"
 #define N_VALIDATORS 4
 #define N_IOTDEVICE 8
 // #include "ns3/mpi-interface.h"
 // #define MPI_TEST
 //
 // #ifdef NS3_MPI
 // #include <mpi.h>
 // #endif

 using namespace ns3;

 double get_wall_time();
 enum ManufacturerID getRandomManufacturerID (void);
 enum ManufacturerID getRandomManufacturerEnum(uint32_t n);
 //int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr);

 NS_LOG_COMPONENT_DEFINE ("PrivateSimpleExperiment");

 int
 main (int argc, char *argv[])
 {
   //#ifdef NS3_MPI
     double tStart = get_wall_time(), tStartSimulation, tFinish;
     const int secsPerMin = 60;
     const uint16_t m_commPort = 5555;
     const double realAverageBlockGenIntervalMinutes = 10; //minutes
     double averageBlockGenIntervalSeconds = 10 * secsPerMin; //seconds
     int start = 0;

     int minConnectionsPerNode = -1;
     int maxConnectionsPerNode = -1;
     enum ManufacturerID manufacturers[N_VALIDATORS+1];

     double averageBlockGenIntervalMinutes = averageBlockGenIntervalSeconds/secsPerMin;
     double stop;

     Ipv4InterfaceContainer                               ipv4InterfaceContainer;
     std::map<uint32_t, std::vector<Ipv4Address>>         nodesConnections;
     std::map<uint32_t, std::map<Ipv4Address, double>>    peersDownloadSpeeds;
     std::map<uint32_t, std::map<Ipv4Address, double>>    peersUploadSpeeds;
     std::map<uint32_t, nodeInternetSpeeds>               nodesInternetSpeeds;
     std::vector<uint32_t>                                validators;
     std::map<uint32_t, uint32_t>                         iotValidatorMap;
     uint32_t                                                  nodesInSystemId0 = 0;
     uint32_t                                             totalNoNodes=0;

     Time::SetResolution (Time::NS);

     uint32_t systemId = 0;
     uint32_t systemCount = 1;

     for(int i=1;i<=N_VALIDATORS;i++){
      validators.push_back(i);
      manufacturers[i] = getRandomManufacturerID();
      totalNoNodes++;
    }

    for(int i=1;i<=N_IOTDEVICE;i++){
      uint32_t validatorId = rand()%N_VALIDATORS;
      uint32_t deviceId = N_VALIDATORS+i;
      iotValidatorMap[deviceId] = validatorId;
     totalNoNodes++;
   }

    // for(int i=1;i<=N_VALIDATORS;i++){
    //   uint32_t gateway = i*N_VALIDATORS+1;
    //   std::vector<uint32_t> childs;
    //   totalNoNodes++;
    //   for(int j=1;j<=4;j++){
    //     childs.push_back(gateway+j);
    //     totalNoNodes++;
    //   }
    //   gatewayChildMap[gateway] = childs;
    //   iotValidatorMap[gateway] = i;
    // }


    NS_LOG_INFO ("Creating Topology");
     IoTFlatTopologyHelper IoTFlatTopologyHelper (systemCount, totalNoNodes, manufacturers,
                                                  minConnectionsPerNode,
                                                  maxConnectionsPerNode, 0, systemId, validators, iotValidatorMap);

    InternetStackHelper stack;
    IoTFlatTopologyHelper.InstallStack (stack);
    // Assign Addresses to Grid
    IoTFlatTopologyHelper.AssignIpv4Addresses (Ipv4AddressHelperCustom ("1.0.0.0", "255.255.255.0", false));
    ipv4InterfaceContainer = IoTFlatTopologyHelper.GetIpv4InterfaceContainer();
    nodesConnections = IoTFlatTopologyHelper.GetNodesConnectionsIps();
    validators = IoTFlatTopologyHelper.GetValidators();
    peersDownloadSpeeds = IoTFlatTopologyHelper.GetPeersDownloadSpeeds();
    peersUploadSpeeds = IoTFlatTopologyHelper.GetPeersUploadSpeeds();
    nodesInternetSpeeds = IoTFlatTopologyHelper.GetNodesInternetSpeeds();

    BlockchainValidatorHelper blockchainValidatorHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), m_commPort),nodesConnections[validators[0]], validators.size(), peersDownloadSpeeds[0], peersUploadSpeeds[0],nodesInternetSpeeds[0], 5.0);

    ApplicationContainer blockchainValidators;

    for(auto &miner : validators)
    {
    	Ptr<Node> targetNode = IoTFlatTopologyHelper.GetNode (miner);
      blockchainValidatorHelper.SetPeersAddresses (nodesConnections[miner]);
      blockchainValidatorHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
      blockchainValidatorHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);
      blockchainValidatorHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[miner]);
      blockchainValidators.Add(blockchainValidatorHelper.Install (targetNode));
    }
    blockchainValidators.Start (Seconds (start));
    blockchainValidators.Stop (Minutes (stop));

    BlockchainNodeHelper blockchainNodeHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), m_commPort),nodesConnections[0], peersDownloadSpeeds[0],  peersUploadSpeeds[0], nodesInternetSpeeds[0]);

    ApplicationContainer blockchainNodes;

    //Install gateway nodes
    tStartSimulation = get_wall_time();
    Simulator::Stop (Minutes (stop + 0.1));
    Simulator::Run ();
    Simulator::Destroy ();
   return 0;
 }

enum ManufacturerID getRandomManufacturerID(void)
{
  int idx = rand()%6;
  return getRandomManufacturerEnum(idx);
}


enum ManufacturerID getRandomManufacturerEnum(uint32_t n)
{
  switch (n)
  {
    case 0: return SAMSUNG;
    case 1: return QUALCOMM;
    case 2: return CISCO;
    case 3: return ERICSSON;
    case 4: return IBM;
    case 5: return SILICON;
    case 6: return OTHER;
  }
}

 double get_wall_time()
 {
     struct timeval time;
     if (gettimeofday(&time,NULL)){
         //  Handle error
         return 0;
     }
     return (double)time.tv_sec + (double)time.tv_usec * .000001;
 }