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

#include "DataFormatsTPC/CalibdEdxCorrection.h"

#include <algorithm>
#include <string_view>

// o2 includes
#include "DataFormatsTPC/Defs.h"
#include "CommonUtils/TreeStreamRedirector.h"

// root includes
#include "TFile.h"

using namespace o2::tpc;

void CalibdEdxCorrection::clear()
{
  for (auto& row : mParams) {
    for (auto& x : row) {
      x = 0.f;
    }
  }
  for (auto& x : mChi2) {
    x = 0.f;
  }
  mDims = -1;
}

void CalibdEdxCorrection::writeToFile(std::string_view fileName, std::string_view objName) const
{
  std::unique_ptr<TFile> file(TFile::Open(fileName.data(), "recreate"));
  file->WriteObject(this, objName.data());
}

void CalibdEdxCorrection::loadFromFile(std::string_view fileName, std::string_view objName)
{
  std::unique_ptr<TFile> file(TFile::Open(fileName.data()));
  auto tmp = file->Get<CalibdEdxCorrection>(objName.data());
  if (tmp != nullptr) {
    *this = *tmp;
  }
}

void CalibdEdxCorrection::dumpToTree(const char* outFileName) const
{
  o2::utils::TreeStreamRedirector pcstream(outFileName, "RECREATE");
  pcstream.GetFile()->cd();

  for (int sector = 0; sector < 2 * SECTORSPERSIDE; ++sector) {
    for (int roc = 0; roc < GEMSTACKSPERSECTOR; ++roc) {
      tpc::StackID stack{
        sector,
        static_cast<tpc::GEMstack>(roc)};

      std::vector<float> qMaxCorrOut;
      std::vector<float> qTotCorrOut;
      std::vector<float> tglOut;
      std::vector<float> snpOut;

      for (float tgl = 0; tgl < 2; tgl += 0.01) {
        for (float snp = 0; snp < 1; snp += 0.1) {
          qMaxCorrOut.emplace_back(getCorrection(stack, ChargeType::Max, tgl, snp));
          qTotCorrOut.emplace_back(getCorrection(stack, ChargeType::Tot, tgl, snp));
          tglOut.emplace_back(tgl);
          snpOut.emplace_back(snp);
        }
      }

      pcstream << "tree"
               << "qMaxCorr=" << qMaxCorrOut
               << "qTotCorr=" << qTotCorrOut
               << "tgl=" << tglOut
               << "snp=" << snpOut
               << "roc=" << roc
               << "sector=" << sector
               << "\n";
    }
  }
}

const std::array<float, CalibdEdxCorrection::ParamSize> CalibdEdxCorrection::getMeanParams(ChargeType charge) const
{
  std::array<float, ParamSize> params{};

  for (int index = 0; index < FitSize / 2; ++index) {
    std::transform(params.begin(), params.end(), mParams[index + charge * FitSize / 2], params.begin(), std::plus<>());
  }
  std::for_each(params.begin(), params.end(), [](auto& val) { val /= (0.5f * FitSize); });
  return params;
}

const std::array<float, CalibdEdxCorrection::ParamSize> CalibdEdxCorrection::getMeanParams(const GEMstack stack, ChargeType charge) const
{
  std::array<float, ParamSize> params{};

  for (int index = 0; index < SECTORSPERSIDE * SIDES; ++index) {
    std::transform(params.begin(), params.end(), getParams(StackID{index, stack}, charge), params.begin(), std::plus<>());
  }
  std::for_each(params.begin(), params.end(), [](auto& val) { val /= (SECTORSPERSIDE * SIDES); });
  return params;
}

float CalibdEdxCorrection::getMeanParam(ChargeType charge, uint32_t param) const
{
  if (param >= ParamSize) {
    return 0.f;
  }
  float mean{};
  for (int index = 0; index < FitSize / 2; ++index) {
    mean += mParams[index + charge * FitSize / 2][param];
  }

  return mean / (0.5f * FitSize);
}

float CalibdEdxCorrection::getMeanParam(const GEMstack stack, ChargeType charge, uint32_t param) const
{
  if (param >= ParamSize) {
    return 0.f;
  }
  float mean{};
  for (int index = 0; index < SECTORSPERSIDE * SIDES; ++index) {
    mean += getParams(StackID{index, stack}, charge)[param];
  }

  return mean / (SECTORSPERSIDE * SIDES);
}

float CalibdEdxCorrection::getMeanEntries(ChargeType charge) const
{
  float mean{};
  for (int index = 0; index < FitSize / 2; ++index) {
    mean += mEntries[index + charge * FitSize / 2];
  }

  return mean / (0.5f * FitSize);
}

float CalibdEdxCorrection::getMeanEntries(const GEMstack stack, ChargeType charge) const
{
  float mean{};
  for (int index = 0; index < SECTORSPERSIDE * SIDES; ++index) {
    mean += getEntries(StackID{index, stack}, charge);
  }

  return mean / (SECTORSPERSIDE * SIDES);
}
