// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#if !defined(__CLING__) || defined(__ROOTCLING__)
#include <numeric>
#include <vector>
#include <string>
#include <string_view>

#include "TFile.h"
#include "TROOT.h"

#include "TPCBase/CDBInterface.h"
#include "TPCBase/Mapper.h"
#include "TPCBase/CalDet.h"
#include "TPCBase/Utils.h"
#include "TPCBase/CRUCalibHelpers.h"
#endif

using namespace o2::tpc;
using namespace o2::tpc::cru_calib_helpers;

/// \param sigmaNoiseROCType can be either one value for all ROC types, or {IROC, OROC}, or {IROC, OROC1, OROC2, OROC3}
/// \param minADCROCType can be either one value for all ROC types, or {IROC, OROC}, or {IROC, OROC1, OROC2, OROC3}
void preparePedestalFiles(const std::string_view pedestalFile, std::string outputDir = "./", std::vector<float> sigmaNoiseROCType = {3}, std::vector<float> minADCROCType = {2}, float pedestalOffset = 0, bool onlyFilled = false, bool maskBad = true, float noisyChannelThreshold = 1.5, float sigmaNoiseNoisyChannels = 4, float badChannelThreshold = 6)
{
  const auto& mapper = Mapper::instance();

  // ===| load noise and pedestal from file |===
  CalDet<float> output("Pedestals");
  const CalDet<float>* calPedestal = nullptr;
  const CalDet<float>* calNoise = nullptr;

  if (pedestalFile.find("cdb") != std::string::npos) {
    auto& cdb = CDBInterface::instance();
    if (pedestalFile.find("cdb-test") == 0) {
      cdb.setURL("http://ccdb-test.cern.ch:8080");
    } else if (pedestalFile.find("cdb-prod") == 0) {
      cdb.setURL("http://alice-ccdb.cern.ch");
    }
    const auto timePos = pedestalFile.find("@");
    if (timePos != std::string_view::npos) {
      std::cout << "set time stamp " << std::stol(pedestalFile.substr(timePos + 1).data()) << "\n";
      cdb.setTimeStamp(std::stol(pedestalFile.substr(timePos + 1).data()));
    }
    calPedestal = &cdb.getPedestals();
    calNoise = &cdb.getNoise();
  } else {
    TFile f(pedestalFile.data());
    gROOT->cd();
    f.GetObject("Pedestals", calPedestal);
    f.GetObject("Noise", calNoise);
  }

  DataMapU32 pedestalValues;
  DataMapU32 thresholdlValues;
  DataMapU32 pedestalValuesPhysics;
  DataMapU32 thresholdlValuesPhysics;

  auto pedestalsThreshold = preparePedestalFiles(*calPedestal, *calNoise, sigmaNoiseROCType, minADCROCType, pedestalOffset, onlyFilled, maskBad, noisyChannelThreshold, sigmaNoiseNoisyChannels, badChannelThreshold);

  // ===| prepare values |===
  for (size_t iroc = 0; iroc < calPedestal->getData().size(); ++iroc) {
    const ROC roc(iroc);

    const auto& rocPedestal = calPedestal->getCalArray(iroc);
    const auto& rocNoise = calNoise->getCalArray(iroc);
    auto& rocOut = output.getCalArray(iroc);

    const int padOffset = roc.isOROC() ? mapper.getPadsInIROC() : 0;

    // skip empty
    if (!(std::abs(rocPedestal.getSum() + rocNoise.getSum()) > 0)) {
      continue;
    }

    // loop over pads
    for (size_t ipad = 0; ipad < rocPedestal.getData().size(); ++ipad) {
      const int globalPad = ipad + padOffset;
      const FECInfo& fecInfo = mapper.fecInfo(globalPad);
      const CRU cru = mapper.getCRU(roc.getSector(), globalPad);
      const uint32_t region = cru.region();
      const int cruID = cru.number();
      const int sampa = fecInfo.getSampaChip();
      const int sampaChannel = fecInfo.getSampaChannel();
      // int globalLinkID = fecInfo.getIndex();

      const PartitionInfo& partInfo = mapper.getMapPartitionInfo()[cru.partition()];
      const int nFECs = partInfo.getNumberOfFECs();
      const int fecOffset = (nFECs + 1) / 2;
      const int fecInPartition = fecInfo.getIndex() - partInfo.getSectorFECOffset();
      const int dataWrapperID = fecInPartition >= fecOffset;
      const int globalLinkID = (fecInPartition % fecOffset) + dataWrapperID * 12;

      const auto pedestal = pedestalsThreshold["Pedestals"].getCalArray(iroc).getValue(ipad);
      const auto threshold = pedestalsThreshold["ThresholdMap"].getCalArray(iroc).getValue(ipad);
      const auto pedestalHighNoise = pedestalsThreshold["PedestalsPhys"].getCalArray(iroc).getValue(ipad);
      const auto thresholdHighNoise = pedestalsThreshold["ThresholdMapPhys"].getCalArray(iroc).getValue(ipad);

      const int hwChannel = getHWChannel(sampa, sampaChannel, region % 2);
      // for debugging
      // printf("%4d %4d %4d %4d %4d: %u\n", cru.number(), globalLinkID, hwChannel, fecInfo.getSampaChip(), fecInfo.getSampaChannel(), getADCValue(pedestal));

      // default thresholds
      const auto adcPedestal = floatToFixedSize(pedestal);
      const auto adcThreshold = floatToFixedSize(threshold);
      pedestalValues[LinkInfo(cruID, globalLinkID)][hwChannel] = adcPedestal;
      thresholdlValues[LinkInfo(cruID, globalLinkID)][hwChannel] = adcThreshold;

      // higher thresholds for physics data taking
      const auto adcPedestalPhysics = floatToFixedSize(pedestalHighNoise);
      const auto adcThresholdPhysics = floatToFixedSize(thresholdHighNoise);
      pedestalValuesPhysics[LinkInfo(cruID, globalLinkID)][hwChannel] = adcPedestalPhysics;
      thresholdlValuesPhysics[LinkInfo(cruID, globalLinkID)][hwChannel] = adcThresholdPhysics;
      // for debugging
      // if(!(std::abs(pedestal - fixedSizeToFloat(adcPedestal)) <= 0.5 * 0.25)) {
      // printf("%4d %4d %4d %4d %4d: %u %.2f %.4f %.4f\n", cru.number(), globalLinkID, hwChannel, sampa, sampaChannel, adcPedestal, fixedSizeToFloat(adcPedestal), pedestal, pedestal - fixedSizeToFloat(adcPedestal));
      //}
    }
  }

  // text files
  const auto outFilePedestalTXT(outputDir + "/pedestal_values.txt");
  const auto outFileThresholdTXT(outputDir + "/threshold_values.txt");

  const auto outFilePedestalPhysTXT(outputDir + "/pedestal_values.physics.txt");
  const auto outFileThresholdPhyTXT(outputDir + "/threshold_values.physics.txt");

  writeValues(outFilePedestalTXT, pedestalValues, onlyFilled);
  writeValues(outFileThresholdTXT, thresholdlValues, onlyFilled);

  writeValues(outFilePedestalPhysTXT, pedestalValuesPhysics, onlyFilled);
  writeValues(outFileThresholdPhyTXT, thresholdlValuesPhysics, onlyFilled);

  // root files
  const auto outFilePedestalROOT(outputDir + "/pedestal_values.root");
  const auto outFileThresholdROOT(outputDir + "/threshold_values.root");

  const auto outFilePedestalPhysROOT(outputDir + "/pedestal_values.physics.root");
  const auto outFileThresholdPhyROOT(outputDir + "/threshold_values.physics.root");

  getCalPad(outFilePedestalTXT, outFilePedestalROOT, "Pedestals");
  getCalPad(outFileThresholdTXT, outFileThresholdROOT, "ThresholdMap");

  getCalPad(outFilePedestalPhysTXT, outFilePedestalPhysROOT, "Pedestals");
  getCalPad(outFileThresholdPhyTXT, outFileThresholdPhyROOT, "ThresholdMap");
}
