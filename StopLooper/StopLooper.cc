// C++
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <cmath>
#include <sstream>
#include <stdexcept>

// ROOT
#include "TDirectory.h"
#include "TTreeCache.h"
#include "Math/VectorUtil.h"
#include "TVector2.h"
#include "TBenchmark.h"
#include "TMath.h"
#include "TString.h"
#include "TLorentzVector.h"
#include "TF1.h"

// CORE
#include "../CORE/Tools/utils.h"
#include "../CORE/Tools/goodrun.h"
#include "../CORE/Tools/dorky/dorky.h"
#include "../CORE/Tools/badEventFilter.h"

// Stop baby class
#include "../StopCORE/StopTree.h"
#include "../StopCORE/TopTagger/ResolvedTopMVA.h"
// #include "../StopCORE/stop_variables/metratio.cc"

#include "SR.h"
#include "StopRegions.h"
#include "StopLooper.h"
#include "Utilities.h"

using namespace std;
using namespace stop1l;

class SR;

const bool verbose = false;
// turn on to apply btag sf - take from files defined in eventWeight_btagSF.cc
const bool applyBtagSFfromFiles = false; // default false
// turn on to apply lepton sf to central value - reread from files
const bool applyLeptonSFfromFiles = false; // default false
// turn on to enable plots of metbins with systematic variations applied. will only do variations for applied weights
const bool doSystVariations = false;
// turn on to enable plots of metbins with different gen classifications
const bool doGenClassification = true;
// turn on to apply Nvtx reweighting to MC / data2016
const bool doNvtxReweight = false;
// turn on top tagging studies, off for baby ver < 25
const bool doTopTagging = true;
// turn on to apply json file to data
const bool applyGoodRunList = true;
// re-run resolved top MVA locally
const bool runResTopMVA = false;
// only produce yield histos
const bool runYieldsOnly = false;
// only running selected signal points to speed up
const bool runFullSignalScan = false;
// debug symbol, for printing exact event kinematics that passes
const bool printPassedEvents = false;

// some global helper variables to be used in member functions
int datayear = -1;
string samplever;

const float fInf = std::numeric_limits<float>::max();

const float kSMSMassStep = 25;
const vector<float> mStopBins = []() { vector<float> bins; for (float i = 150; i < 1350; i += kSMSMassStep) bins.push_back(i); return bins; } ();
const vector<float> mLSPBins  = []() { vector<float> bins; for (float i =   0; i <  750; i += kSMSMassStep) bins.push_back(i); return bins; } ();

std::ofstream ofile;

void StopLooper::SetSignalRegions() {

  // SRVec = getStopSignalRegionsTopological();
  // CR0bVec = getStopControlRegionsNoBTagsTopological();
  // CR2lVec = getStopControlRegionsDileptonTopological();

  SRVec = getStopSignalRegionsNewMETBinning();
  CR0bVec = getStopControlRegionsNoBTagsNewMETBinning();
  CR2lVec = getStopControlRegionsDileptonNewMETBinning();

  // SRVec = getStopInclusiveRegionsTopological();
  // CR0bVec = getStopInclusiveControlRegionsNoBTags();
  // CR2lVec = getStopInclusiveControlRegionsDilepton();

  CRemuVec = getStopCrosscheckRegionsEMu();

  if (verbose) {
    cout << "SRVec.size = " << SRVec.size() << ", including the following:" << endl;
    for (auto it = SRVec.begin(); it != SRVec.end(); ++it) {
      cout << it-SRVec.begin() << "  " << it->GetName() << ":  " << it->GetDetailName() << endl;
    }
  }

  auto createRangesHists = [&] (vector<SR>& srvec) {
    for (auto& sr : srvec) {
      vector<string> vars = sr.GetListOfVariables();
      TDirectory * dir = (TDirectory*) outfile_->Get((sr.GetName() + "/ranges").c_str());
      if (dir == 0) dir = outfile_->mkdir((sr.GetName() + "/ranges").c_str());
      dir->cd("ranges");
      for (auto& var : vars) {
        plot1d("h_"+var+"_"+"LOW",  1, sr.GetLowerBound(var), sr.histMap, "", 1, 0, 2);
        plot1d("h_"+var+"_"+"HI",   1, sr.GetUpperBound(var), sr.histMap, "", 1, 0, 2);
      }
      if (sr.GetNMETBins() > 0) {
        plot1d("h_metbins", -1, 0, sr.histMap, (sr.GetName()+"_"+sr.GetDetailName()+";E^{miss}_{T} [GeV]").c_str(), sr.GetNMETBins(), sr.GetMETBinsPtr());
      }
    }
  };

  createRangesHists(SRVec);
  createRangesHists(CR0bVec);
  createRangesHists(CR2lVec);
  createRangesHists(CRemuVec);

  testVec.emplace_back("testGeneral");
  testVec.emplace_back("testTopTagging");
  testVec.emplace_back("testCutflow");
}


void StopLooper::GenerateAllSRptrSets() {
  allSRptrSets.clear();

  vector<SR*> all_SRptrs;
  for (auto& sr : SRVec)   all_SRptrs.push_back(&sr);
  for (auto& cr : CR2lVec) all_SRptrs.push_back(&cr);
  for (auto& cr : CR0bVec) all_SRptrs.push_back(&cr);

  // Place holder for more effective way of SR finding for each event, which is still under development
  // allSRptrSets = generateSRptrSet(all_SRptrs);
}

bool StopLooper::PassingHLTriggers(const int type) {
  if (type == 1) {
    switch (year_) {
      case 2016:
        return ( (HLT_MET110_MHT110() || HLT_MET120_MHT120() || HLT_MET()) ||
                 (abs(lep1_pdgid()) == 11 && HLT_SingleEl()) || (abs(lep1_pdgid()) == 13 && HLT_SingleMu()) );
      case 2017:
      case 2018:
      default:
        return ( HLT_MET_MHT() || (abs(lep1_pdgid()) == 11 && HLT_SingleEl()) || (abs(lep1_pdgid()) == 13 && HLT_SingleMu()) );
    }
  } else if (type == 2) {
    switch (year_) {
      case 2016:
        return ( (HLT_MET() || HLT_MET110_MHT110() || HLT_MET120_MHT120()) || (HLT_SingleEl() && (abs(lep1_pdgid())==11 || abs(lep2_pdgid())==11)) ||
                 (HLT_SingleMu() && (abs(lep1_pdgid())==13 || abs(lep2_pdgid())==13)) );
      case 2017:
      case 2018:
      default:
        return ( HLT_MET_MHT() || (HLT_SingleEl() && (abs(lep1_pdgid())==11 || abs(lep2_pdgid())==11)) ||
                  (HLT_SingleMu() && (abs(lep1_pdgid())==13 || abs(lep2_pdgid())==13)) );
    }
  } else if (type == 3) {
    int dilepid = abs(lep1_pdgid()*lep2_pdgid());
    switch (year_) {
      case 2016:
        return ( (HLT_MET() || HLT_MET110_MHT110() || HLT_MET120_MHT120()) ||
                 (dilepid == 121 && HLT_DiEl()) || (dilepid == 169 && HLT_DiMu()) || (dilepid == 143 && HLT_MuE()) ||
                 (HLT_SingleEl() && (abs(lep1_pdgid())==11 || abs(lep2_pdgid())==11)) ||
                 (HLT_SingleMu() && (abs(lep1_pdgid())==13 || abs(lep2_pdgid())==13)) );
      case 2017:
      case 2018:
      default:
        return ( HLT_MET_MHT() || (dilepid == 121 && HLT_DiEl()) || (dilepid == 169 && HLT_DiMu()) || (dilepid == 143 && HLT_MuE()) ||
                 (HLT_SingleEl() && (abs(lep1_pdgid())==11 || abs(lep2_pdgid())==11)) ||
                 (HLT_SingleMu() && (abs(lep1_pdgid())==13 || abs(lep2_pdgid())==13)) );
    }
  }
  return false;
}

void StopLooper::looper(TChain* chain, string samplestr, string output_dir, int jes_type) {

  // Benchmark
  TBenchmark *bmark = new TBenchmark();
  bmark->Start("benchmark");

  TString output_name = Form("%s/%s.root",output_dir.c_str(),samplestr.c_str());
  cout << "[StopLooper::looper] creating output file: " << output_name << endl;  outfile_ = new TFile(output_name.Data(),"RECREATE") ;
  cout << "Complied with C++ standard: " << __cplusplus << endl;

  if (printPassedEvents) ofile.open("passEventList.txt");

  if (runResTopMVA)
    resTopMVA = new ResolvedTopMVA("../StopCORE/TopTagger/resTop_xGBoost_v2.weights.xml", "BDT");

  outfile_ = new TFile(output_name.Data(), "RECREATE") ;

  float kLumi = 133.53;
  // Combined 2016 (35.922/fb), 2017 (41.529/fb) and 2018 (56.077/fb) json,
  const char* json_file = "../StopCORE/inputs/json_files/Cert_271036-325175_13TeV_Combined161718_JSON_snt.txt";

  // Determine datayear from the sample name
  if (samplestr == "data_single_lepton_met") datayear = 16;
  else if (samplestr.find("data_2016") == 0) datayear = 2016;
  else if (samplestr.find("data_2017") == 0) datayear = 2017;
  else if (samplestr.find("data_2018") == 0) datayear = 2018;
  else datayear = -1;

  // Setup pileup re-weighting for comparing data of different years
  if (doNvtxReweight) {
    TFile f_purw("../AnalyzeScripts/pu_reweighting_hists/nvtx_reweighting_alldata.root");
    TString scaletype = "18to17";
    if (datayear == 2018) scaletype = "18to17";
    else if (datayear == 2016) scaletype = "16to17";
    TH1F* h_nvtxscale = (TH1F*) f_purw.Get("h_nvtxscale_"+scaletype);
    if (!h_nvtxscale) throw invalid_argument("No nvtx reweighting hist found for " + scaletype);
    if (verbose) cout << "Doing nvtx reweighting! Scaling " << scaletype << ". The scale factors are:" << endl;
    for (int i = 1; i < 100; ++i) {
      nvtxscale_[i] = h_nvtxscale->GetBinContent(i);
      if (verbose) cout << i << "  " << nvtxscale_[i] << endl;
    }
  }

  if (applyGoodRunList) {
    cout << "Loading json file: " << json_file << endl;
    set_goodrun_file(json_file);
  }

  TFile dummy( (output_dir+"/dummy.root").c_str(), "RECREATE" );
  SetSignalRegions();
  // GenerateAllSRptrSets();

  int nDuplicates = 0;
  int nEvents = chain->GetEntries();
  unsigned int nEventsChain = nEvents;
  cout << "[StopLooper::looper] running on " << nEventsChain << " events" << endl;
  unsigned int nEventsTotal = 0;
  unsigned int nPassedTotal = 0;

  TObjArray *listOfFiles = chain->GetListOfFiles();
  TIter fileIter(listOfFiles);
  TFile *currentFile = 0;
  while ( (currentFile = (TFile*)fileIter.Next()) ) {

    TString fname = currentFile->GetTitle();
    TFile file( fname, "READ" );
    TTree *tree = (TTree*) file.Get("t");
    TTreeCache::SetLearnEntries(10);
    tree->SetCacheSize(128*1024*1024);
    babyAnalyzer.Init(tree);

    // Use the first event to get dsname
    tree->LoadTree(0);
    babyAnalyzer.GetEntry(0);

    // Setup configs for sample dependent processes
    year_ = (doTopTagging)? year() : 2016;
    TString dsname = dataset();

    // Find the stopbaby versions automatically from file path
    if (int i = fname.Index("_v"); i >= 0) samplever = fname(i+1, 3); // ignore subversions
    else if (fname.Contains("v24")) samplever = "v24";
    else cout << "[looper] >> Cannot find the sample version!" << endl;

    // Attach the MiniAOD version from dsname
    if (dsname.Contains("RunIIFall17MiniAODv2")) samplever += ":Fall17v2";
    else if (dsname.Contains("RunIISummer16MiniAODv2")) samplever += ":Summer16v2";
    else if (dsname.Contains("RunIISummer16MiniAODv3")) samplever += ":Summer16v3";
    else if (dsname.Contains("RunIISpring16MiniAODv2")) samplever += ":Spring16v2";

    cout << "[looper] >> Running on sample: " << dsname << endl;
    cout << "[looper] >> Sample detected with year = " << year_ << " and version = " << samplever << endl;

    is_fastsim_ = fname.Contains("SMS") || fname.Contains("Signal");

    // Get event weight histogram from baby
    TH3D* h_sig_counter = nullptr;
    TH2D* h_sig_counter_nEvents = nullptr;
    if (is_fastsim_) {
      h_sig_counter = (TH3D*) file.Get("h_counterSMS");
      h_sig_counter_nEvents = (TH2D*) file.Get("histNEvts");
    }

    // Setup the event weight calculator
    evtWgt.verbose = true;
    if (year_ == 2016)
      evtWgt.setDefaultSystematics(0);  // systematic set for Moriond17 analysis
    else if (year_ >= 2017)
      evtWgt.setDefaultSystematics(1);  // systematic set for 94X
    evtWgt.Setup(samplestr, year_, doSystVariations, applyBtagSFfromFiles, applyLeptonSFfromFiles);

    evtWgt.getCounterHistogramFromBaby(&file);
    // Extra file weight for extension dataset, should move these code to other places
    if (year_ == 2016 && samplever.find("v22") == 0)
      evtWgt.getExtSampleWeightSummer16v2(fname);
    else if (year_ == 2016 && samplever.find("Summer16v3") != string::npos)
      evtWgt.getExtSampleWeightSummer16v3(fname);
    else if (year_ == 2017 && samplever.find("v29") == 0)
      evtWgt.getExtSampleWeightFall17v2(fname);

    evtWgt.getZSampleWeightFromCR3l(fname);

    if (year_ == 2016) kLumi = 35.867;
    else if (year_ == 2017) kLumi = 41.96;
    else if (year_ == 2018) kLumi = 70;

    dummy.cd();
    // Loop over Events in current file
    if (nEventsTotal >= nEventsChain) continue;
    unsigned int nEventsTree = tree->GetEntriesFast();
    for (unsigned int event = 0; event < nEventsTree; ++event) {
      // Read Tree
      if (nEventsTotal >= nEventsChain) continue;
      tree->LoadTree(event);
      babyAnalyzer.GetEntry(event);
      ++nEventsTotal;

      if ( is_data() ) {
        if ( applyGoodRunList && !goodrun(run(), ls()) ) continue;
        duplicate_removal::DorkyEventIdentifier id(run(), evt(), ls());
        if ( is_duplicate(id) ) {
          ++nDuplicates;
          continue;
        }
      }

      // fillEfficiencyHistos(testVec[0], "filters");

      // Apply met filters
      if (doTopTagging) {
        // Recommended filters for the legacy analysis
        switch (year_) {
          case 2018:  // 2017 and 2018 uses the same set of filters
          case 2017:
            if ( !filt_ecalbadcalib() ) continue;
          case 2016:
            if ( !filt_goodvtx() ) continue;
            if ( !is_fastsim_ && !filt_globalsupertighthalo2016() ) continue;
            if ( !filt_hbhenoise() ) continue;
            if ( !filt_hbheisonoise() )   continue;
            if ( !filt_ecaltp() ) continue;
            if ( !filt_badMuonFilter() ) continue;
            if ( !filt_badChargedCandidateFilter() ) continue;
            if ( is_data() && !filt_eebadsc() ) continue;
        }
      } else if (samplever.find("v24") == 0) {
        // Filters used in Moriond17 study, keep for sync check
        if ( !filt_met() ) continue;
        if ( !filt_goodvtx() ) continue;
        if ( firstGoodVtxIdx() == -1 ) continue;
        if ( !filt_badMuonFilter() ) continue;
        if ( !filt_badChargedCandidateFilter() ) continue;
        if ( !filt_jetWithBadMuon() ) continue;
        if ( !filt_pfovercalomet() ) continue;
        if ( filt_duplicatemuons() ) continue; // Temporary, breaks for old 16 babies
        if ( filt_badmuons() ) continue;
        if ( !filt_nobadmuons() ) continue;
      }

      // stop defined filters
      if (is_fastsim_) {
        if ( !filt_fastsimjets() ) continue;
      } else {
        if ( !filt_pfovercalomet() ) continue;  // reject event if pfmet/calomet > 5
        if ( !filt_jetWithBadMuon() ) continue; // there's a jup/jdown of this filter, but this alone should be enough
      }

      // Require at least 1 good vertex
      if (nvtxs() < 1) continue;

      // Fill tirgger efficiency histos after the MET filters are applied
      // fillEfficiencyHistos(testVec[0], "triggers");

      // For testing on only subset of mass points
      if (!runFullSignalScan && is_fastsim_) {
        // auto checkMassPt = [&](double mstop, double mlsp) { return (mass_stop() == mstop) && (mass_lsp() == mlsp); };
        // if (!checkMassPt(800,400)  && !checkMassPt(1200,50)  && !checkMassPt(400,100) &&
        //     !checkMassPt(1100,300) && !checkMassPt(1100,500) && !checkMassPt(900,600)) continue;
        if (dsname.Contains("T2tt")) {
          float massdiff = mass_stop() - mass_lsp();
          // if (mass_lsp() < 400 && mass_stop() < 1000) continue;
          // if (massdiff < 200 || massdiff > 400) continue;
          // if (massdiff < 200 || massdiff > 400 || mass_lsp() < 400) continue;
          if (massdiff < 600 || mass_stop() < 1000) continue;
          plot2d("h2d_T2tt_masspts", mass_stop(), mass_lsp(), 1, SRVec.at(0).histMap, ";M_{stop} [GeV]; M_{lsp} [GeV]", 100, 300, 1300, 80, 0, 800);
        } else if (dsname.Contains("T2bW")) {
          float massdiff = mass_stop() - mass_lsp();
          if (mass_lsp() < 300 && mass_stop() < 800) continue;
          if (massdiff < 300) continue;
          // if (massdiff < 900 || mass_stop() < 1000) continue;
          plot2d("h2d_T2bW_masspts", mass_stop(), mass_lsp(), 1, SRVec.at(0).histMap, ";M_{stop} [GeV]; M_{lsp} [GeV]", 100, 300, 1300, 80, 0, 800);
        } else if (dsname.Contains("T2bt")) {
          float massdiff = mass_stop() - mass_lsp();
          if (mass_lsp() < 300 && mass_stop() < 800) continue;
          if (massdiff < 300) continue;
          // if (massdiff < 900 || mass_stop() < 1000) continue;
          // if (!checkMassPt(800, 400) && !checkMassPt(800, 600) && !checkMassPt(1000, 50) && !checkMassPt(1000, 200)) continue;
          plot2d("h2d_T2bt_masspts", mass_stop(), mass_lsp(), 1, SRVec.at(0).histMap, ";M_{stop} [GeV]; M_{LSP} [GeV]", 100, 300, 1300, 64, 0, 800);
        }
      }

      // Only consider events with nupt < 200 for the inclusive WNJetsToLNu samples
      if (dsname.BeginsWith("/W") && dsname.Contains("JetsToLNu") && !dsname.Contains("NuPt-200") && nupt() > 200) continue;

      if (is_fastsim_) {
        if (fmod(mass_stop(), kSMSMassStep) > 2 || fmod(mass_lsp(), kSMSMassStep) > 2) continue;  // skip points in between the binning
        plot2d("h2d_signal_masspts", mass_stop(), mass_lsp() , evtweight_, SRVec.at(0).histMap, ";M_{stop} [GeV]; M_{lsp} [GeV]", 96, 100, 1300, 64, 0, 800);
      }

      ++nPassedTotal;

      is_bkg_ = (!is_data() && !is_fastsim_);

      // Calculate event weight
      evtWgt.resetEvent(); // full event weights only get calculated if the event get selected for a SR

      // Simple weight with scale1fb only
      if (!is_data()) {
        if (is_fastsim_) {
          int nEventsSample = h_sig_counter_nEvents->GetBinContent(h_sig_counter->FindBin(mass_stop(), mass_lsp()));
          evtweight_ = kLumi * xsec() * 1000 / nEventsSample;
        } else {
          evtweight_ = kLumi * scale1fb();
        }
      }

      // Plot nvtxs on the base selection of stopbaby for reweighting purpose
      plot1d("h_nvtxs", nvtxs(), 1, testVec[0].histMap, ";Number of vertices", 100, 1, 101);

      if (doNvtxReweight && (datayear == 2016 || datayear == 2018)) {
        if (nvtxs() < 100) evtweight_ = nvtxscale_[nvtxs()];  // only scale for data
        plot1d("h_nvtxs_rwtd", nvtxs(), evtweight_, testVec[0].histMap, ";Number of vertices", 100, 1, 101);
      }

      // nbtag for CSV valued btags -- for comparison with the Moriond17 analysis
      int nbtagCSV = 0;
      int ntbtagCSV = 0;
      for (float csv : ak4pfjets_CSV()) {
        if (year_ == 2016) {
          if (csv > 0.8484) nbtagCSV++;  // 80X Moriond17
          if (csv > 0.9535) ntbtagCSV++; // 80X Moriond17
        } else if (year_ == 2017) {
          if (csv > 0.8838) nbtagCSV++;  // 94X
          if (csv > 0.9693) ntbtagCSV++; // 94X
        }
      }

      // Calculate leading top tagger variables for the event
      float lead_restopdisc = -1.1;
      float lead_tftopdisc = -0.1;
      float lead_deepdisc_top = -0.1;
      float lead_bindisc_top = -0.1;
      if (doTopTagging) {
        lead_restopdisc = (topcands_disc().size())? topcands_disc()[0] : -1.1;
        for (size_t iak8 = 0; iak8 < ak8pfjets_deepdisc_top().size(); ++iak8) {
          float disc = ak8pfjets_deepdisc_top()[iak8];
          if (disc > lead_deepdisc_top) lead_deepdisc_top = disc;
          float bindisc = disc / (disc + ak8pfjets_deepdisc_qcd().at(iak8));
          if (bindisc > lead_bindisc_top) lead_bindisc_top = bindisc;
        }
        for (auto disc : tftops_disc()) {
          if (disc > lead_tftopdisc) lead_tftopdisc = disc;
        }
      }

      // Filling the variables for analysis
      values_.clear();

      /// Common variables for all JES type
      values_["nlep"] = ngoodleps();
      values_["nvlep"] = nvetoleps();
      values_["lep1pt"] = lep1_p4().pt();
      values_["passvetos"] = PassTrackVeto() && PassTauVeto();

      // For toptagging, add correct switch later
      // values_["resttag"] = lead_restopdisc;
      values_["resttag"] = lead_tftopdisc;
      values_["deepttag"] = lead_deepdisc_top;
      values_["tfttag"] = lead_tftopdisc;
      values_["bdtttag"] = lead_restopdisc;
      values_["binttag"] = lead_bindisc_top;

      if (runResTopMVA) {
        // Prepare deep_cvsl vector
        vector<float> ak4pfjets_dcvsl;
        for (size_t j = 0; j < ak4pfjets_deepCSV().size(); ++j) {
          ak4pfjets_dcvsl.push_back(ak4pfjets_deepCSVc().at(j) / (ak4pfjets_deepCSVc().at(j) + ak4pfjets_deepCSVl().at(j)));
        }
        resTopMVA->setJetVecPtrs(&ak4pfjets_p4(), &ak4pfjets_deepCSV(), &ak4pfjets_dcvsl, &ak4pfjets_ptD(), &ak4pfjets_axis1(), &ak4pfjets_mult());
        std::vector<TopCand> topcands = resTopMVA->getTopCandidates(-1);
        values_["resttag"] = (topcands.size() > 0)? topcands[0].disc : -1.1;
      }

      // Temporary test for top tagging efficiency
      // testTopTaggingEffficiency(testVec[1]);

      /// Values only for hist filling or testing
      values_["chi2"] = hadronic_top_chi2();
      values_["lep1eta"] = lep1_p4().eta();
      values_["passlep1pt"] = (abs(lep1_pdgid()) == 13 && lep1_p4().pt() > 40) || (abs(lep1_pdgid()) == 11 && lep1_p4().pt() > 45);

      for (int jestype = 0; jestype < ((doSystVariations && !is_data())? 3 : 1); ++jestype) {
        if (doSystVariations) jestype_ = jestype;
        string suffix = "";

        /// JES type dependent variables
        if (jestype_ == 0) {
          values_["mt"] = mt_met_lep();
          values_["met"] = pfmet();
          values_["mlb"] = Mlb_closestb();
          values_["tmod"] = topnessMod();
          values_["njet"] = ngoodjets();
          values_["nbjet"] = ngoodbtags();
          values_["nbtag"]  = nanalysisbtags();
          values_["dphijmet"] = mindphi_met_j1_j2();
          values_["dphilmet"] = lep1_dphiMET();
          values_["j1passbtag"] = (ngoodjets() > 0)? ak4pfjets_passMEDbtag().at(0) : 0;

          values_["jet1pt"] = (ngoodjets() > 0)? ak4pfjets_p4().at(0).pt() : 0;
          values_["jet2pt"] = (ngoodjets() > 1)? ak4pfjets_p4().at(1).pt() : 0;
          values_["jet1eta"] = (ngoodjets() > 0)? ak4pfjets_p4().at(0).eta() : -9;
          values_["jet2eta"] = (ngoodjets() > 1)? ak4pfjets_p4().at(1).eta() : -9;

          values_["ht"] = ak4_HT();
          values_["metphi"] = pfmet_phi();
          values_["ntbtag"] = ntightbtags();
          values_["leadbpt"] = ak4pfjets_leadbtag_p4().pt();
          values_["mlb_0b"] = (ak4pfjets_leadbtag_p4() + lep1_p4()).M();
          // values_["htratio"] = ak4_htratiom();

          // suffix = "_nominal";
        } else if (jestype_ == 1) {
          values_["mt"] = mt_met_lep_jup();
          values_["met"] = pfmet_jup();
          values_["mlb"] = Mlb_closestb_jup();
          values_["tmod"] = topnessMod_jup();
          values_["njet"] = jup_ngoodjets();
          values_["nbjet"] = jup_ngoodbtags();  // nbtag30();
          values_["nbtag"]  = jup_nanalysisbtags();
          values_["dphijmet"] = mindphi_met_j1_j2_jup();
          values_["dphilmet"] = fabs(lep1_p4().phi() - pfmet_phi_jup());
          values_["j1passbtag"] = (jup_ngoodjets() > 0)? jup_ak4pfjets_passMEDbtag().at(0) : 0;

          values_["jet1pt"] = (jup_ngoodjets() > 0)? jup_ak4pfjets_p4().at(0).pt() : 0;
          values_["jet2pt"] = (jup_ngoodjets() > 1)? jup_ak4pfjets_p4().at(1).pt() : 0;
          values_["jet1eta"] = (jup_ngoodjets() > 0)? jup_ak4pfjets_p4().at(0).eta() : -9;
          values_["jet2eta"] = (jup_ngoodjets() > 1)? jup_ak4pfjets_p4().at(1).eta() : -9;

          values_["ht"] = jup_ak4_HT();
          values_["metphi"] = pfmet_phi_jup();
          values_["ntbtag"] = jup_ntightbtags();
          values_["leadbpt"] = jup_ak4pfjets_leadbtag_p4().pt();
          values_["mlb_0b"] = (jup_ak4pfjets_leadbtag_p4() + lep1_p4()).M();
          // values_["htratio"] = jup_ak4_htratiom();

          suffix = "_jesUp";
        } else if (jestype_ == 2) {
          values_["mt"] = mt_met_lep_jdown();
          values_["met"] = pfmet_jdown();
          values_["mlb"] = Mlb_closestb_jdown();
          values_["tmod"] = topnessMod_jdown();
          values_["njet"] = jdown_ngoodjets();
          values_["nbjet"] = jdown_ngoodbtags();  // nbtag30();
          values_["ntbtag"] = jdown_ntightbtags();
          values_["dphijmet"] = mindphi_met_j1_j2_jdown();
          values_["dphilmet"] = fabs(lep1_p4().phi() - pfmet_phi_jdown());
          values_["j1passbtag"] = (jdown_ngoodjets() > 0)? jdown_ak4pfjets_passMEDbtag().at(0) : 0;

          values_["jet1pt"] = (jdown_ngoodjets() > 0)? jdown_ak4pfjets_p4().at(0).pt() : 0;
          values_["jet2pt"] = (jdown_ngoodjets() > 1)? jdown_ak4pfjets_p4().at(1).pt() : 0;
          values_["jet1eta"] = (jdown_ngoodjets() > 0)? jdown_ak4pfjets_p4().at(0).eta() : -9;
          values_["jet2eta"] = (jdown_ngoodjets() > 1)? jdown_ak4pfjets_p4().at(1).eta() : -9;

          values_["ht"] = jdown_ak4_HT();
          values_["metphi"] = pfmet_phi_jdown();
          values_["nbtag"]  = jdown_nanalysisbtags();
          values_["leadbpt"] = jdown_ak4pfjets_leadbtag_p4().pt();
          values_["mlb_0b"] = (jdown_ak4pfjets_leadbtag_p4() + lep1_p4()).M();
          // values_["htratio"] = jdown_ak4_htratiom();

          suffix = "_jesDn";
        }
        /// should do the same job as nanalysisbtags
        values_["nbtag"] = (values_["mlb"] > 175)? values_["ntbtag"] : values_["nbjet"];

        // // Uncomment following lines if want to use CSV instead
        // values_["nbtag"] = (values_["mlb"] > 175)? ntbtagCSV : nbtagCSV;
        // values_["nbjet"] = nbtagCSV;
        // values_["ntbtag"] = ntbtagCSV;

        // Filling histograms for SR
        fillHistosForSR(suffix);

        fillHistosForCR0b(suffix);

        // Filling analysis variables with removed leptons, for CR2l
        values_["nlep_rl"] = (ngoodleps() == 1 && nvetoleps() >= 2 && lep2_p4().Pt() > 10)? 2 : ngoodleps();
        values_["mll"] = (lep1_p4() + lep2_p4()).M();

        if (jestype_ == 0) {
          values_["mt_rl"] = mt_met_lep_rl();
          values_["mt2_ll"] = (doTopTagging)? MT2_ll() : 90;
          values_["met_rl"] = pfmet_rl();
          values_["dphijmet_rl"]= mindphi_met_j1_j2_rl();
          values_["dphilmet_rl"] = lep1_dphiMET_rl();
          values_["tmod_rl"] = topnessMod_rl();
        } else if (jestype_ == 1) {
          values_["mt_rl"] = mt_met_lep_rl_jup();
          values_["mt2_ll"] = (doTopTagging)? MT2_ll_jup() : 90;
          values_["met_rl"] = pfmet_rl_jup();
          values_["dphijmet_rl"]= mindphi_met_j1_j2_rl_jup();
          values_["dphilmet_rl"] = lep1_dphiMET_rl_jup();
          values_["tmod_rl"] = topnessMod_rl_jup();
        } else if (jestype_ == 2) {
          values_["mt_rl"] = mt_met_lep_rl_jdown();
          values_["mt2_ll"] = (doTopTagging)? MT2_ll_jdown() : 90;
          values_["met_rl"] = pfmet_rl_jdown();
          values_["dphijmet_rl"]= mindphi_met_j1_j2_rl_jdown();
          values_["dphilmet_rl"] = lep1_dphiMET_rl_jdown();
          values_["tmod_rl"] = topnessMod_rl_jdown();
        }
        fillHistosForCR2l(suffix);
        fillHistosForCRemu(suffix);

        // testCutFlowHistos(testVec[2]);
        fillTopTaggingHistos(suffix);

        // // Also do yield using genmet for fastsim samples <-- under development
        // if (is_fastsim_ && jestype_ == 0) {
        //   values_["met"] = genmet();
        //   values_["mt"] = mt_genmet_lep();
        //   values_["tmod"] = topnessMod_genmet();
        //   values_["dphijmet"] = mindphi_genmet_j1_j2();
        //   values_["dphilmet"] = mindphi_genmet_lep1();

        //   fillHistosForSR("_genmet");
        // }

      }  // end of jes variation

      // if (event > 10) break;  // for debugging purpose
    } // end of event loop


    delete tree;
    file.Close();
  } // end of file loop

  cout << "[StopLooper::looper] processed  " << nEventsTotal << " events" << endl;
  if ( nEventsChain != nEventsTotal )
    cout << "WARNING: Number of events from files is not equal to total number of events" << endl;

  outfile_->cd();

  auto writeHistsToFile = [&] (vector<SR>& srvec) {
    for (auto& sr : srvec) {
      TDirectory* dir = (TDirectory*) outfile_->Get(sr.GetName().c_str());
      if (dir == 0) dir = outfile_->mkdir(sr.GetName().c_str()); // shouldn't happen
      dir->cd();
      for (auto& h : sr.histMap) {
        if (h.first.find("HI") != string::npos || h.first.find("LOW") != string::npos) continue;
        // Move overflows of the yield hist to the last bin of histograms
        if (h.first.find("h_metbins") != string::npos)
          moveOverFlowToLastBin1D(h.second);
        else if (h.first.find("hSMS_metbins") != string::npos)
          moveXOverFlowToLastBin3D((TH3*) h.second);
        h.second->Write();
      }
    }
  };

  writeHistsToFile(testVec);
  writeHistsToFile(SRVec);
  writeHistsToFile(CR0bVec);
  writeHistsToFile(CR2lVec);
  writeHistsToFile(CRemuVec);

  auto writeRatioHists = [&] (const SR& sr) {
    for (const auto& h : sr.histMap) {
      if (h.first.find("hnum") != 0) continue;
      string hname = h.first;
      hname.erase(0, 4);
      dummy.cd();
      TH1F* h_ratio = (TH1F*) h.second->Clone(("ratio"+hname).c_str());
      h_ratio->Divide(h_ratio, sr.histMap.at("hden"+hname), 1, 1, "B");

      TDirectory * dir = (TDirectory*) outfile_->Get(sr.GetName().c_str());
      if (dir == 0) dir = outfile_->mkdir(sr.GetName().c_str()); // shouldn't happen
      dir->cd();
      h_ratio->Write();
    }
  };
  writeRatioHists(testVec[0]);
  writeRatioHists(testVec[1]);

  outfile_->Write();
  outfile_->Close();
  if (printPassedEvents) ofile.close();

  bmark->Stop("benchmark");
  cout << endl;
  cout << nEventsTotal << " Events Processed, where " << nDuplicates << " duplicates were skipped, and ";
  cout << nPassedTotal << " Events passed all selections." << endl;
  cout << "------------------------------" << endl;
  cout << "CPU  Time:   " << Form( "%.01f s", bmark->GetCpuTime("benchmark")  ) << endl;
  cout << "Real Time:   " << Form( "%.01f s", bmark->GetRealTime("benchmark") ) << endl;
  cout << endl;
  delete bmark;

  return;
}

void StopLooper::fillYieldHistos(SR& sr, float met, string suf, bool is_cr2l) {

  evtweight_ = evtWgt.getWeight(evtWgtInfo::systID(jestype_), is_cr2l);

  if (doNvtxReweight && (datayear == 2016 || datayear == 2018)) {
    if (nvtxs() < 100) evtweight_ *= nvtxscale_[nvtxs()];  // only scale for data
  }

  auto fillhists = [&](string s) {
    if (is_fastsim_) {
      plot3d("hSMS_metbins"+s+suf, met, mass_stop(), mass_lsp(), evtweight_, sr.histMap, ";E^{miss}_{T} [GeV];M_{stop};M_{LSP}",
             sr.GetNMETBins(), sr.GetMETBinsPtr(), mStopBins.size()-1, mStopBins.data(), mLSPBins.size()-1, mLSPBins.data());
    } else {
      plot1d("h_metbins"+s+suf, met, evtweight_, sr.histMap, ";E^{miss}_{T} [GeV]" , sr.GetNMETBins(), sr.GetMETBinsPtr());
    }
    if (doSystVariations && (is_bkg_ || is_fastsim_) && jestype_ == 0) {
      // Only run once when JES type == 0. JES variation dealt with above. No need for signals?
      for (int isyst = 3; isyst < evtWgtInfo::k_nSyst; ++isyst) {
        auto syst = (evtWgtInfo::systID) isyst;
        if (evtWgt.doingSystematic(syst)) {
          if (is_fastsim_)
            plot3d("hSMS_metbins"+s+"_"+evtWgt.getLabel(syst), met, mass_stop(), mass_lsp(), evtweight_, sr.histMap, ";E^{miss}_{T} [GeV];M_{stop};M_{LSP}",
                   sr.GetNMETBins(), sr.GetMETBinsPtr(), mStopBins.size()-1, mStopBins.data(), mLSPBins.size()-1, mLSPBins.data());
          else
            plot1d("h_metbins"+s+"_"+evtWgt.getLabel(syst), met, evtWgt.getWeight(syst, is_cr2l), sr.histMap, ";E^{miss}_{T} [GeV]", sr.GetNMETBins(), sr.GetMETBinsPtr());
        }
      }
    }
  };
  fillhists("");
  if (doGenClassification && is_bkg_) {
    // Only fill gen classification for background events, used for background estimation
    // fillhists("_allclass");
    if (isZtoNuNu()) fillhists("_Znunu");
    else if (is2lep()) fillhists("_2lep");
    else if (is1lepFromW()) fillhists("_1lepW");
    else if (is1lepFromTop()) fillhists("_1lepTop");
    else fillhists("_unclass");  // either unclassified 1lep or 0lep, or something else unknown, shouldn't have (m)any
  }

  // Block for debugging, active when setting printPassedEvents = true
  if (printPassedEvents && sr.GetName() == "srbase" && suf == "") {
    // static bool printOnce1 = false;
    // if (!printOnce1) { for (auto& v : values_) ofile << ' ' << v.first; ofile << endl; printOnce1 = true; }
    ofile << run() << ' ' << ls() << ' ' << evt() << ' ' << sr.GetName();
    // if (run() == 277168) for (auto& v : values_) ofile << ' ' << v.second;
    ofile << ": pfmet()= " << pfmet() << ", ngoodjets()= " << ngoodjets() << ", ak4pfjets_p4()[1].pt()= " << ak4pfjets_p4()[1].pt() << endl;
    // ofile << ' ' << evtweight_ << ' ' << evtWgt.getWeight(evtWgtInfo::systID::k_bTagEffHFUp) << ' ' << evtWgt.getLabel(evtWgtInfo::systID::k_bTagEffHFUp);
    // ofile << ' ' << evtweight_ << ' ' << pfmet() << ' ' << mt_met_lep() << ' ' << Mlb_closestb() << ' ' << topnessMod() << ' ' << ngoodjets() << ' ' << ngoodbtags() << ' ' << mindphi_met_j1_j2() << ' ' << pfmet_rl();
    ofile << endl;
  }
}

void StopLooper::fillHistosForSR(string suf) {

  // Trigger requirements
  if (is_data() && !PassingHLTriggers()) return;

  // // For getting into full trigger efficiency in 2017 data
  // if ( (abs(lep1_pdgid()) == 11 && values_["lep1pt"] < 40) || (abs(lep1_pdgid()) == 13 && values_["lep1pt"] < 30) ) return;

  for (auto& sr : SRVec) {
    if (!sr.PassesSelection(values_)) continue;
    fillYieldHistos(sr, values_["met"], suf);

    if (runYieldsOnly) continue;

    // Plot kinematics histograms
    auto fillKineHists = [&](string s) {
      // Simple plot function plot1d to add extra plots anywhere in the code, is great for quick checks
      plot1d("h_mt"+s,       values_["mt"]      , evtweight_, sr.histMap, ";M_{T} [GeV]"             , 12,  150, 600);
      plot1d("h_met"+s,      values_["met"]     , evtweight_, sr.histMap, ";#slash{E}_{T} [GeV]"     , 24, 250, 850);
      plot1d("h_metphi"+s,   values_["metphi"]  , evtweight_, sr.histMap, ";#phi(#slash{E}_{T})"     , 34, -3.4, 3.4);
      plot1d("h_lep1pt"+s,   values_["lep1pt"]  , evtweight_, sr.histMap, ";p_{T}(lepton) [GeV]"     , 24,  0, 600);
      plot1d("h_lep1eta"+s,  values_["lep1eta"] , evtweight_, sr.histMap, ";#eta(lepton)"            , 30, -3, 3);
      plot1d("h_nleps"+s,    values_["nlep"]    , evtweight_, sr.histMap, ";Number of leptons"       ,  5,  0, 5);
      plot1d("h_njets"+s,    values_["njet"]    , evtweight_, sr.histMap, ";Number of jets"          ,  8,  2, 10);
      plot1d("h_nbjets"+s,   values_["nbjet"]   , evtweight_, sr.histMap, ";Number of b-tagged jets" ,  4,  1, 5);
      plot1d("h_mlepb"+s,    values_["mlb"]     , evtweight_, sr.histMap, ";M_{#it{l}b} [GeV]"  , 24,  0, 600);
      plot1d("h_dphijmet"+s, values_["dphijmet"], evtweight_, sr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 25,  0.8, 3.3);
      plot1d("h_tmod"+s,     values_["tmod"]    , evtweight_, sr.histMap, ";Modified topness"        , 25, -10, 15);
      plot1d("h_nvtxs"+s,        nvtxs()        , evtweight_, sr.histMap, ";Number of vertices"      , 100,  1, 101);

      // if ( (HLT_SingleEl() && abs(lep1_pdgid()) == 11 && values_["lep1pt"] < 45) || (HLT_SingleMu() && abs(lep1_pdgid()) == 13 && values_["lep1pt"] < 40) || (HLT_MET_MHT() && pfmet() > 250) ) {
      if (true) {
        plot1d("h_mt_h"+s,       values_["mt"]       , evtweight_, sr.histMap, ";M_{T} [GeV]"                   , 24,   0, 600);
        plot1d("h_met_h"+s,      values_["met"]      , evtweight_, sr.histMap, ";#slash{E}_{T} [GeV]"           , 24,  50, 650);
        plot1d("h_nbtags"+s,     values_["nbjet"]    , evtweight_, sr.histMap, ";Number of b-tagged jets"       ,  5,  0, 5);
        plot1d("h_dphijmet_h"+s, values_["dphijmet"] , evtweight_, sr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33,  0.0, 3.3);
      }

      plot1d("h_jet1pt"+s,  values_["jet1pt"],  evtweight_, sr.histMap, ";p_{T}(jet1) [GeV]"  , 32,  0, 800);
      plot1d("h_jet2pt"+s,  values_["jet2pt"],  evtweight_, sr.histMap, ";p_{T}(jet2) [GeV]"  , 32,  0, 800);
      plot1d("h_jet1eta"+s, values_["jet1eta"], evtweight_, sr.histMap, ";#eta(jet1) [GeV]"   , 30,  -3,  3);
      plot1d("h_jet2eta"+s, values_["jet2eta"], evtweight_, sr.histMap, ";#eta(jet2) [GeV]"   , 60,  -3,  3);
    };
    // if (sr.GetName().find("base") != string::npos) // only plot for base regions
    if (suf == "") fillKineHists(suf);
    auto checkMassPt = [&](double mstop, double mlsp) { return (mass_stop() == mstop) && (mass_lsp() == mlsp); };
    if (is_fastsim_ && suf == "" && (checkMassPt(1200, 50) || checkMassPt(800, 400)))
      fillKineHists("_"+to_string((int)mass_stop())+"_"+to_string((int)mass_lsp()) + suf);

    // // Re-using fillKineHists with different suffix for extra/checking categories
    // if ( abs(lep1_pdgid()) == 11 )
    //   fillKineHists(suf+"_e");
    // else if ( abs(lep1_pdgid()) == 13 )
    //   fillKineHists(suf+"_mu");

    // if ( HLT_SingleMu() ) fillKineHists(suf+"_hltmu");
    // if ( HLT_SingleEl() ) fillKineHists(suf+"_hltel");
    // if ( HLT_MET_MHT() )  fillKineHists(suf+"_hltmet");

    // if (sr.GetName().find("srI") == 0) {
    //   double coeff[3];
    //   double Rmin(0), Rmax(0);
    //   LorentzVector jisr = ak4pfjets_p4().at(0);
    //   for (size_t j = 1; j < ak4pfjets_p4().size(); ++j) {
    //     if ( isCloseObject(ak4pfjets_p4().at(0), ak4pfjets_p4().at(j), 2) &&
    //          !isCloseObject(lep1_p4(), ak4pfjets_p4().at(j), 1) ) {
    //       jisr += ak4pfjets_p4().at(j);
    //     }
    //   }
    //   bool sign = calculateMETRatioWCorridor(pfmet(), pfmet_phi(), jisr, lep1_p4(), Rmin, Rmax, coeff);
    //   Rmin = (Rmin < 0)? 0 : (Rmin > 6)? 5.99 : Rmin;
    //   Rmax = (Rmax < 0)? 0 : (Rmax > 6)? 5.99 : Rmax;
    //   plot1d("h_Rm_sign",   sign , evtweight_, sr.histMap, ";sgn(#Delta) for #bar{R}_{M}(W)"   , 2, 0, 2);
    //   plot1d("h_Rm_logA",   log(fabs(coeff[0])) , evtweight_, sr.histMap, ";log(A) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rm_logB",   log(fabs(coeff[1])) , evtweight_, sr.histMap, ";log(B) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rm_logC",   log(fabs(coeff[2])) , evtweight_, sr.histMap, ";log(C) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rmin_auto", Rmin , evtweight_, sr.histMap, ";#bar{R}_{min}"   , 60, 0, 6);
    //   if (sign) {
    //     plot1d("h_Rm_width", Rmax - Rmin , evtweight_, sr.histMap, ";#sqrt(#Delta)/2A for #bar{R}_{M}(W)" , 50, 0, 20);
    //     plot1d("h_Rmin",   Rmin , evtweight_, sr.histMap, ";#bar{R}_{min}"  , 60, 0, 6);
    //     plot1d("h_Rmax",   Rmax , evtweight_, sr.histMap, ";#bar{R}_{max}"  , 60, 0, 6);
    //     plot1d("h_Rcen_all",  (Rmin+Rmax)/2 , evtweight_, sr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //   } else {
    //     plot1d("h_Rcen_all",   Rmin , evtweight_, sr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //     plot1d("h_Rcen_fail",  Rmin , evtweight_, sr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //   }
    // }

  }
  // SRVec[0].PassesSelectionPrintFirstFail(values_);
}

void StopLooper::fillHistosForCR2l(string suf) {

  // Trigger requirements
  if (is_data() && !PassingHLTriggers(2)) return;

  // For getting into full trigger efficiency in 2017 & 2018 data
  // if (not( (HLT_SingleEl() && abs(lep1_pdgid()) == 11 && values_["lep1pt"] < 45) ||
  //          (HLT_SingleMu() && abs(lep1_pdgid()) == 13 && values_["lep1pt"] < 40) || (HLT_MET_MHT() && pfmet() > 250) )) return;

  for (auto& cr : CR2lVec) {
    if (!cr.PassesSelection(values_)) continue;
    fillYieldHistos(cr, values_["met_rl"], suf, true);

    if (runYieldsOnly) continue;

    auto fillKineHists = [&] (string s) {
      plot1d("h_finemet"+s,  values_["met"]         , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"           , 80,  0, 800);
      plot1d("h_met"+s,      values_["met"]         , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"           , 20, 250, 650);
      plot1d("h_metphi"+s,   values_["metphi"]      , evtweight_, cr.histMap, ";#phi(#slash{E}_{T})"           , 40,  -4, 4);
      plot1d("h_mt"+s,       values_["mt"]          , evtweight_, cr.histMap, ";M_{T} [GeV]"                   , 12,  150, 600);
      plot1d("h_rlmet"+s,    values_["met_rl"]      , evtweight_, cr.histMap, ";#slash{E}_{T} (with removed lepton) [GeV]" , 20, 250, 650);
      plot1d("h_rlmt"+s,     values_["mt_rl"]       , evtweight_, cr.histMap, ";M_{T} (with removed lepton) [GeV]" , 10,  150, 600);
      plot1d("h_tmod"+s,     values_["tmod_rl"]     , evtweight_, cr.histMap, ";Modified topness"              , 20, -10, 15);
      plot1d("h_njets"+s,    values_["njet"]        , evtweight_, cr.histMap, ";Number of jets"                ,  8,  2, 10);
      plot1d("h_nbjets"+s,   values_["nbjet"]       , evtweight_, cr.histMap, ";Number of b-tagged jets"       ,  4,  1, 5);
      plot1d("h_nleps"+s,    values_["nlep_rl"]     , evtweight_, cr.histMap, ";nleps (dilep)"                 ,  5,  0, 5);
      plot1d("h_lep1pt"+s,   values_["lep1pt"]      , evtweight_, cr.histMap, ";p_{T}(lepton) [GeV]"           , 24,  0, 600);
      plot1d("h_lep1eta"+s,  values_["lep1eta"]     , evtweight_, cr.histMap, ";#eta(lepton)"                  , 30, -3, 3);
      plot1d("h_mlepb"+s,    values_["mlb"]         , evtweight_, cr.histMap, ";M_{#it{l}b} [GeV]"             , 24,  0, 600);
      plot1d("h_dphijmet"+s, values_["dphijmet_rl"] , evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33,  0, 3.3);
      plot1d("h_nvtxs"+s,          nvtxs()          , evtweight_, cr.histMap, ";Number of vertices"            , 70,  1, 71);

      // if ( (HLT_SingleEl() && abs(lep1_pdgid()) == 11 && values_["lep1pt"] < 45) || (HLT_SingleMu() && abs(lep1_pdgid()) == 13 && values_["lep1pt"] < 40) || (HLT_MET_MHT() && pfmet() > 250) ) {
      if (true) {
        plot1d("h_met_h"+s,      values_["met"]      , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"           , 24,  50, 650);
        plot1d("h_mt_h"+s,       values_["mt"]       , evtweight_, cr.histMap, ";M_{T} [GeV]"                   , 24,   0, 600);
        plot1d("h_rlmet_h"+s,    values_["met_rl"]   , evtweight_, cr.histMap, ";#slash{E}_{T} (with removed lepton) [GeV]" , 24, 50, 650);
        plot1d("h_rlmt_h"+s,     values_["mt_rl"]    , evtweight_, cr.histMap, ";M_{T} (with removed lepton) [GeV]" , 12, 0, 600);
        plot1d("h_nbtags"+s,     values_["nbjet"]    , evtweight_, cr.histMap, ";Number of b-tagged jets"       ,  5,  0, 5);
        plot1d("h_dphijmet_h"+s, values_["dphijmet"] , evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})"  , 33, 0.0, 3.3);
        plot1d("h_dphijmet_notrl"+s, values_["dphijmet"], evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33,  0, 3.3);
      }

      plot1d("h_jet1pt"+s,  values_["jet1pt"],  evtweight_, cr.histMap, ";p_{T}(jet1) [GeV]"  , 32,  0, 800);
      plot1d("h_jet2pt"+s,  values_["jet2pt"],  evtweight_, cr.histMap, ";p_{T}(jet2) [GeV]"  , 32,  0, 800);
      plot1d("h_jet1eta"+s, values_["jet1eta"], evtweight_, cr.histMap, ";#eta(jet1) [GeV]"   , 30,  -3,  3);
      plot1d("h_jet2eta"+s, values_["jet2eta"], evtweight_, cr.histMap, ";#eta(jet2) [GeV]"   , 60,  -3,  3);

      // Luminosity test at Z peak
      if (lep1_pdgid() == -lep2_pdgid()) {
        plot1d("h_mll"+s,   values_["mll"], evtweight_, cr.histMap, ";M_{#it{ll}} [GeV]" , 120, 0, 240 );
        if (82 < values_["mll"] && values_["mll"] < 100) {
          plot1d("h_zpt"+s, (lep1_p4() + lep2_p4()).pt(), evtweight_, cr.histMap, ";p_{T}(Z) [GeV]"          , 200, 0, 200);
          plot1d("h_njets_zpeak"+s,  values_["njet"]    , evtweight_, cr.histMap, ";Number of jets"          , 12,  0, 12);
          plot1d("h_nbjets_zpeak"+s, values_["nbjet"]   , evtweight_, cr.histMap, ";Number of b-tagged jets" ,  6,  0, 6);
        } else {
          plot1d("h_njets_noz"+s,    values_["njet"]    , evtweight_, cr.histMap, ";Number of jets"          , 12,  0, 12);
          plot1d("h_nbjets_noz"+s,   values_["nbjet"]   , evtweight_, cr.histMap, ";Number of b-tagged jets" ,  6,  0, 6);
        }
      }
    };
    fillKineHists(suf);

    // if ( HLT_SingleMu() ) fillKineHists(suf+"_hltmu");
    // if ( HLT_SingleEl() ) fillKineHists(suf+"_hltel");
    // if ( HLT_MET_MHT() )  fillKineHists(suf+"_hltmet");

    // if ( abs(lep1_pdgid()*lep2_pdgid()) == 121 )
    //   fillKineHists(suf+"_ee");
    // else if ( abs(lep1_pdgid()*lep2_pdgid()) == 143 )
    //   fillKineHists(suf+"_emu");
    // else if ( abs(lep1_pdgid()*lep2_pdgid()) == 169 )
    //   fillKineHists(suf+"_mumu");

  }
}

void StopLooper::fillHistosForCR0b(string suf) {

  // Trigger requirements
  if (is_data() && !PassingHLTriggers()) return;

  for (auto& cr : CR0bVec) {
    if (!cr.PassesSelection(values_)) continue;
    fillYieldHistos(cr, values_["met"], suf);

    if (runYieldsOnly) continue;

    auto fillKineHists = [&] (string s) {
      plot1d("h_mt"+s,       values_["mt"]      , evtweight_, cr.histMap, ";M_{T} [GeV]"          , 12, 150, 600);
      plot1d("h_met"+s,      values_["met"]     , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"  , 24, 250, 650);
      plot1d("h_metphi"+s,   values_["metphi"]  , evtweight_, cr.histMap, ";#phi(#slash{E}_{T})"  , 34, -3.4, 3.4);
      plot1d("h_lep1pt"+s,   values_["lep1pt"]  , evtweight_, cr.histMap, ";p_{T}(lepton) [GeV]"  , 24,  0, 600);
      plot1d("h_lep1eta"+s,  values_["lep1eta"] , evtweight_, cr.histMap, ";#eta(lepton)"         , 30, -3, 3);
      plot1d("h_nleps"+s,    values_["nlep"]    , evtweight_, cr.histMap, ";Number of leptons"    ,  5,  0, 5);
      plot1d("h_njets"+s,    values_["njet"]    , evtweight_, cr.histMap, ";Number of jets"       ,  8,  2, 10);
      plot1d("h_nbjets"+s,   values_["nbjet"]   , evtweight_, cr.histMap, ";Number of b-tagged jets" ,  5,  0, 5);
      plot1d("h_mlepb"+s,    values_["mlb_0b"]  , evtweight_, cr.histMap, ";M_{#it{l}b} [GeV]"  , 24,  0, 600);
      plot1d("h_dphijmet"+s, values_["dphijmet"], evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33,  0, 3.3);
      plot1d("h_tmod"+s,     values_["tmod"]    , evtweight_, cr.histMap, ";Modified topness"     , 25, -10, 15);
      plot1d("h_nvtxs"+s,        nvtxs()        , evtweight_, cr.histMap, ";Number of vertices"   , 70,  1, 71);

      // if ( (HLT_SingleEl() && abs(lep1_pdgid()) == 11 && values_["lep1pt"] < 45) || (HLT_SingleMu() && abs(lep1_pdgid()) == 13 && values_["lep1pt"] < 40) || (HLT_MET_MHT() && pfmet() > 250) ) {
      if (true) {
        plot1d("h_mt_h"+s,       values_["mt"]       , evtweight_, cr.histMap, ";M_{T} [GeV]"                   , 24,   0, 600);
        plot1d("h_met_h"+s,      values_["met"]      , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"           , 24,  50, 650);
        plot1d("h_nbtags"+s,     values_["nbjet"]    , evtweight_, cr.histMap, ";Number of b-tagged jets"       ,  5,   0,   5);
        plot1d("h_dphijmet_h"+s, values_["dphijmet"] , evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33, 0.0, 3.3);
      }

      plot1d("h_jet1pt"+s,  values_["jet1pt"],  evtweight_, cr.histMap, ";p_{T}(jet1) [GeV]"  , 32,  0, 800);
      plot1d("h_jet2pt"+s,  values_["jet2pt"],  evtweight_, cr.histMap, ";p_{T}(jet2) [GeV]"  , 32,  0, 800);
      plot1d("h_jet1eta"+s, values_["jet1eta"], evtweight_, cr.histMap, ";#eta(jet1) [GeV]"   , 30,  -3,  3);
      plot1d("h_jet2eta"+s, values_["jet2eta"], evtweight_, cr.histMap, ";#eta(jet2) [GeV]"   , 60,  -3,  3);
      // Temporary test for low dphijmet excess
      plot1d("h_dphij1j2"+s, fabs(ak4pfjets_p4().at(0).phi()-ak4pfjets_p4().at(1).phi()), evtweight_, cr.histMap, ";#Delta#phi(j1,j2)" , 33,  0, 3.3);
    };
    fillKineHists(suf);

    // if ( abs(lep1_pdgid()) == 11 ) {
    //   fillKineHists(suf+"_e");
    // } else if ( abs(lep1_pdgid()) == 13 ) {
    //   fillKineHists(suf+"_mu");
    // }

    // if (HLT_SingleMu() || HLT_SingleEl())
    //   fillKineHists(suf+"_hltsl");
    // if (HLT_MET_MHT())
    //   fillKineHists(suf+"_hltmet");

    // if (cr.GetName().find("cr0bI") == 0) {
    //   double coeff[3];
    //   double Rmin(0), Rmax(0);
    //   LorentzVector jisr = ak4pfjets_p4().at(0);
    //   for (size_t j = 1; j < ak4pfjets_p4().size(); ++j) {
    //     if ( isCloseObject(ak4pfjets_p4().at(0), ak4pfjets_p4().at(j), 2) &&
    //          !isCloseObject(lep1_p4(), ak4pfjets_p4().at(j), 1) ) {
    //       jisr += ak4pfjets_p4().at(j);
    //     }
    //   }
    //   bool sign = calculateMETRatioWCorridor(pfmet(), pfmet_phi(), jisr, lep1_p4(), Rmin, Rmax, coeff);
    //   Rmin = (Rmin < 0)? 0 : (Rmin > 6)? 5.99 : Rmin;
    //   Rmax = (Rmax < 0)? 0 : (Rmax > 6)? 5.99 : Rmax;
    //   plot1d("h_Rm_sign",   sign , evtweight_, cr.histMap, ";sgn(#Delta) for #bar{R}_{M}(W)"   , 2, 0, 2);
    //   plot1d("h_Rm_logA",   log(fabs(coeff[0])) , evtweight_, cr.histMap, ";log(A) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rm_logB",   log(fabs(coeff[1])) , evtweight_, cr.histMap, ";log(B) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rm_logC",   log(fabs(coeff[2])) , evtweight_, cr.histMap, ";log(C) for #bar{R}_{M}(W)"  , 100, -200, 200);
    //   plot1d("h_Rmin_auto", Rmin , evtweight_, cr.histMap, ";#bar{R}_{min}"   , 60, 0, 6);
    //   if (sign) {
    //     plot1d("h_Rm_width", Rmax - Rmin , evtweight_, cr.histMap, ";#sqrt(#Delta)/2A for #bar{R}_{M}(W)" , 50, 0, 20);
    //     plot1d("h_Rmin",   Rmin , evtweight_, cr.histMap, ";#bar{R}_{min}"  , 60, 0, 6);
    //     plot1d("h_Rmax",   Rmax , evtweight_, cr.histMap, ";#bar{R}_{max}"  , 60, 0, 6);
    //     plot1d("h_Rcen_all",  (Rmin+Rmax)/2 , evtweight_, cr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //   } else {
    //     plot1d("h_Rcen_all",   Rmin , evtweight_, cr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //     plot1d("h_Rcen_fail",  Rmin , evtweight_, cr.histMap, ";#bar{R}_{M}(W)"  , 60, 0, 6);
    //   }
    // }
  }
}

void StopLooper::fillHistosForCRemu(string suf) {

  // Trigger requirements and go to the plateau region
  if ( !HLT_MET_MHT() || pfmet() < 250 ) return;
  if ( !(lep1_pdgid() * lep2_pdgid() == -143) ) return;

  // Basic cuts that are not supposed to change frequently
  if ( lep1_p4().pt() < 30 || fabs(lep1_p4().eta()) > 2.1 ||
       (abs(lep1_pdgid()) == 13 && !lep1_passTightID()) ||
       (abs(lep1_pdgid()) == 11 && !lep1_passMediumID()) ||
       lep1_MiniIso() > 0.1 ) return;

  if ( lep2_p4().pt() < 15 || fabs(lep2_p4().eta()) > 2.1 ||
       (abs(lep2_pdgid()) == 13 && !lep2_passTightID()) ||
       (abs(lep2_pdgid()) == 11 && !lep2_passMediumID()) ||
       lep2_MiniIso() > 0.1 ) return;

  if ( (lep1_p4() + lep2_p4()).M() < 20 ) return;

  values_["lep2pt"] = lep2_p4().pt();
  values_["lep2eta"] = lep2_p4().eta();

  for (auto& cr : CRemuVec) {
    if ( cr.PassesSelection(values_) ) {

      auto fillhists = [&] (string s) {
        plot1d("h_mt"+s,       values_["mt"]      , evtweight_, cr.histMap, ";M_{T} [GeV]"          , 10, 150, 650);
        plot1d("h_mt_h"+s,     values_["mt"]      , evtweight_, cr.histMap, ";M_{T} [GeV]"          , 12,  0, 600);
        plot1d("h_mt2w"+s,     values_["mt2w"]    , evtweight_, cr.histMap, ";MT2W [GeV]"           , 18,  50, 500);
        plot1d("h_met"+s,      values_["met"]     , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"  , 20, 250, 650);
        plot1d("h_met_h"+s,    values_["met"]     , evtweight_, cr.histMap, ";#slash{E}_{T} [GeV]"  , 24,  50, 650);
        plot1d("h_metphi"+s,   values_["metphi"]  , evtweight_, cr.histMap, ";#phi(#slash{E}_{T})"  , 32, -3.2, 3.2);
        plot1d("h_lep1pt"+s,   values_["lep1pt"]  , evtweight_, cr.histMap, ";p_{T}(lepton) [GeV]"  , 24,  0, 600);
        plot1d("h_lep2pt"+s,   values_["lep2pt"]  , evtweight_, cr.histMap, ";p_{T}(lep2) [GeV]"    , 30,  0, 300);
        plot1d("h_lep1eta"+s,  values_["lep1eta"] , evtweight_, cr.histMap, ";#eta(lepton)"         , 30, -3, 3);
        plot1d("h_lep2eta"+s,  values_["lep2eta"] , evtweight_, cr.histMap, ";#eta(lep2)"           , 20, -3, 3);
        plot1d("h_nleps"+s,    values_["nlep"]    , evtweight_, cr.histMap, ";Number of leptons"    ,  5,  0, 5);
        plot1d("h_njets"+s,    values_["njet"]    , evtweight_, cr.histMap, ";Number of jets"       ,  8,  2, 10);
        plot1d("h_nbjets"+s,   values_["nbjet"]   , evtweight_, cr.histMap, ";nbtags"               , 6,   0, 6);
        plot1d("h_tmod"+s,     values_["tmod"]    , evtweight_, cr.histMap, ";Modified topness"     , 30, -15, 15);
        plot1d("h_mlepb"+s,    values_["mlb_0b"]  , evtweight_, cr.histMap, ";M_{#it{l}b} [GeV]" , 24,  0, 600);
        plot1d("h_dphijmet"+s, values_["dphijmet"], evtweight_, cr.histMap, ";#Delta#phi(jet,#slash{E}_{T})" , 33,  0, 3.3);

        plot1d("h_jet1pt"+s,  values_["jet1pt"],  evtweight_, cr.histMap, ";p_{T}(jet1) [GeV]"  , 32,  0, 800);
        plot1d("h_jet2pt"+s,  values_["jet2pt"],  evtweight_, cr.histMap, ";p_{T}(jet2) [GeV]"  , 32,  0, 800);
        plot1d("h_jet1eta"+s, values_["jet1eta"], evtweight_, cr.histMap, ";#eta(jet1) [GeV]"   , 30,  -3,  3);
        plot1d("h_jet2eta"+s, values_["jet2eta"], evtweight_, cr.histMap, ";#eta(jet2) [GeV]"   , 60,  -3,  3);

        const float leppt_bins[] = {0, 30, 40, 50, 75, 100, 125, 200};
        plot1d("h_lep1ptbins"+s, values_["lep1pt"], evtweight_, cr.histMap, ";p_{T}(lepton) [GeV]", 7, leppt_bins);
        plot1d("h_lep2ptbins"+s, values_["lep2pt"], evtweight_, cr.histMap, ";p_{T}(lep2) [GeV]"  , 7, leppt_bins);
      };
      fillhists(suf);
      if (HLT_MuE())
        fillhists(suf+"_passHLT");
    }
  }
}

void StopLooper::fillEfficiencyHistos(SR& sr, const string type, string suffix) {

  // // Temporary for the early 2018 PromptReco data trying to reduce the effect from HCAL issue
  // if (mindphi_met_j1_j2() < 0.5) return;

  // Temporary globalTightHalo filter study
  if (is_data() && (type == "" || type == "filters")) {
    plot1d("hden_met_filt_globaltight"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]", 160, 50, 850);
    plot1d("hden_metphi_filt_globaltight"+suffix, pfmet_phi(), 1, sr.histMap, ";#phi(#slash{E}_{T})", 128, -3.2, 3.2);
    plot1d("hden_met_filt_supertight"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]", 160, 50, 850);
    plot1d("hden_metphi_filt_supertight"+suffix, pfmet_phi(), 1, sr.histMap, ";#phi(#slash{E}_{T})", 128, -3.2, 3.2);

    if (filt_globalsupertighthalo2016()) {
      plot1d("hnum_met_filt_supertight"+suffix, pfmet(), filt_globalsupertighthalo2016(), sr.histMap, ";#slash{E}_{T} [GeV]", 160, 50, 850);
      plot1d("hnum_metphi_filt_supertight"+suffix, pfmet_phi(), filt_globalsupertighthalo2016(), sr.histMap, ";#phi(#slash{E}_{T})", 128, -3.2, 3.2);
    }
    if (filt_globaltighthalo2016()) {
      plot1d("hnum_met_filt_globaltight"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]", 160, 50, 850);
      plot1d("hnum_metphi_filt_globaltight"+suffix, pfmet_phi(), 1, sr.histMap, ";#phi(#slash{E}_{T})", 128, -3.2, 3.2);
    }
  }

  if (is_data() && (type == "" || type == "triggers")) {
    // Study the efficiency of the MET trigger
    if (HLT_SingleMu() && abs(lep1_pdgid()) == 13 && nvetoleps() == 1 && lep1_p4().pt() > 40) {
      plot1d("hden_met_hltmet"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
      plot1d("hden_met_hltmetmht120"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
      plot1d("hden_ht_hltht_unprescaled"+suffix, ak4_HT(), 1, sr.histMap, ";H_{T} [GeV]", 30, 800, 1400);
      if (HLT_MET_MHT())
        plot1d("hnum_met_hltmet"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
      if (HLT_MET120_MHT120())
        plot1d("hnum_met_hltmetmht120"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
      if (HLT_PFHT_unprescaled())
        plot1d("hnum_ht_hltht_unprescaled"+suffix, ak4_HT(), 1, sr.histMap, ";H_{T} [GeV]", 30, 800, 1400);
    } else if (HLT_SingleEl() && abs(lep1_pdgid()) == 11 && nvetoleps() == 1 && lep1_p4().pt() > 45) {
      plot1d("hden_met_hltmet_eden"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
      if (HLT_MET_MHT())
        plot1d("hnum_met_hltmet_eden"+suffix, pfmet(), 1, sr.histMap, ";#slash{E}_{T} [GeV]"  , 60,  50, 650);
    }

    // Study the efficiency of the single lepton triggers
    if (HLT_MET_MHT() && pfmet() > 250) {
      float lep1pt = lep1_p4().pt();
      lep1pt = (lep1pt < 400)? lep1pt : 399;
      if (abs(lep1_pdgid()) == 13) {
        plot1d("hden_lep1pt_hltmu"+suffix, lep1pt, 1, sr.histMap, ";p_{T}(lep1) [GeV]", 50,  0, 400);
        if (HLT_SingleMu())
          plot1d("hnum_lep1pt_hltmu"+suffix, lep1pt, 1, sr.histMap, ";p_{T}(#mu) [GeV]", 50,  0, 400);
      }
      else if (abs(lep1_pdgid()) == 11) {
        plot1d("hden_lep1pt_hltel"+suffix, lep1pt, 1, sr.histMap, ";p_{T}(lep1) [GeV]", 50,  0, 400);
        if (HLT_SingleEl())
          plot1d("hnum_lep1pt_hltel"+suffix, lep1pt, 1, sr.histMap, ";p_{T}(e) [GeV]", 50,  0, 400);
      }
    }

    // Study the OR of the 3 triggers, uses jetht dataset
    if (HLT_PFHT_unprescaled())
      plot1d("h_ht"+suffix, ak4_HT(), 1, sr.histMap, ";H_{T} [GeV];#slash{E}_{T} [GeV]", 30, 800, 1400);

    // Measure the efficiencies of all 3 trigger combined in a JetHT dataset
    // if ((HLT_PFHT_unprescaled() || HLT_PFHT_prescaled())) {
    if ((HLT_PFHT_unprescaled() || HLT_PFHT_prescaled()) || HLT_AK8Jet_unprescaled() || HLT_AK8Jet_prescaled() || HLT_CaloJet500_NoJetID()) {
    // if (true) {
      const float TEbins_met[] = {150, 200, 225, 250, 275, 300, 350, 400, 550};
      const float TEbins_lep[] = {20, 22.5, 25, 30, 40, 55, 100, 200};
      float met = (pfmet() > 550)? 549.9 : pfmet();
      float lep1pt = lep1_p4().pt();
      int lep1id = abs(lep1_pdgid());
      lep1pt = (lep1pt > 200)? 199 : lep1pt;
      plot2d("hden2d_trigeff_met_lep1pt"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
      if      (lep1id == 11) plot2d("hden2d_trigeff_met_lep1pt_el"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
      else if (lep1id == 13) plot2d("hden2d_trigeff_met_lep1pt_mu"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
      if (PassingHLTriggers()) {
        plot2d("hnum2d_trigeff_met_lep1pt"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
        if      (lep1id == 11) plot2d("hnum2d_trigeff_met_lep1pt_el"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1-e) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
        else if (lep1id == 13) plot2d("hnum2d_trigeff_met_lep1pt_mu"+suffix, lep1pt, met, 1, sr.histMap, ";p_{T}(lep1-#mu) [GeV];#slash{E}_{T} [GeV]", 7, TEbins_lep, 8, TEbins_met);
      }
      // Trigger efficiency for CR2l
      if (nvetoleps() > 1) {
        float met_rl = (pfmet_rl() > 550)? 549.9 : pfmet_rl();
        plot2d("hden2d_trigeff_metrl_lep1pt"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
        if      (lep1id == 11) plot2d("hden2d_trigeff_metrl_lep1pt_el"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
        else if (lep1id == 13) plot2d("hden2d_trigeff_metrl_lep1pt_mu"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
        if (PassingHLTriggers(2)) {
          plot2d("hnum2d_trigeff_metrl_lep1pt"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
          if      (lep1id == 11) plot2d("hnum2d_trigeff_metrl_lep1pt_el"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1-e) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
          else if (lep1id == 13) plot2d("hnum2d_trigeff_metrl_lep1pt_mu"+suffix, lep1pt, met_rl, 1, sr.histMap, ";p_{T}(lep1-#mu) [GeV];#slash{E}_{T}(rl) [GeV]", 7, TEbins_lep, 8, TEbins_met);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
// Functions that are not indispensible part of the main analysis

void StopLooper::fillTopTaggingHistos(string suffix) {
  if (!doTopTagging || runYieldsOnly) return;
  if (suffix != "") return;

  bool pass_deeptop_tag = false;
  // float lead_restopdisc = -1.1;
  float lead_deepdisc_top = -0.1;
  float lead_deepdisc_W = -0.1;
  float lead_bindisc_top = -0.1;
  float lead_bindisc_W = -0.1;
  int iak8_top = -1;
  int iak8_W = -1;

  float lead_tftop_disc = -0.1;
  for (auto disc : tftops_disc()) {
    if (disc > lead_tftop_disc) lead_tftop_disc = disc;
  }

  for (size_t iak8 = 0; iak8 < ak8pfjets_deepdisc_top().size(); ++iak8) {
    float disc = ak8pfjets_deepdisc_top()[iak8];
    if (disc > lead_deepdisc_top) {
      lead_deepdisc_top = disc;
      iak8_top = iak8;
    }
    if (disc > 0.7 || (ak8pfjets_p4().at(iak8).pt() > 500 && disc > 0.3))
      pass_deeptop_tag = true;

    float bindisc = disc / (disc + ak8pfjets_deepdisc_qcd().at(iak8));
    if (bindisc > lead_bindisc_top) lead_bindisc_top = bindisc;

    float discW = ak8pfjets_deepdisc_w()[iak8];
    if (discW > lead_deepdisc_W) {
      lead_deepdisc_W = discW;
      iak8_W = iak8;
    }
    float bindiscW = discW / (discW + ak8pfjets_deepdisc_qcd().at(iak8));
    if (bindiscW > lead_bindisc_W) lead_bindisc_W = bindiscW;
  }

  values_["deepWtag"] = lead_deepdisc_W;
  values_["binttag"] = lead_bindisc_top;
  values_["binWtag"] = lead_bindisc_W;
  values_["topak8pt"] = (iak8_top < 0)? 0 : ak8pfjets_p4().at(iak8_top).pt();
  values_["Wak8pt"] = (iak8_W < 0)? 0 : ak8pfjets_p4().at(iak8_W).pt();
  values_["passdeepttag"] = pass_deeptop_tag;
  // values_["passresttag"] = lead_restopdisc > 0.9;
  // values_["ntftops"] = 0;
  values_["ntftops"] = tftops_p4().size();

  auto fillTopTagHists = [&](SR& sr, string s) {
    plot1d("h_nak8jets", ak8pfjets_deepdisc_top().size(), evtweight_, sr.histMap, ";Number of AK8 jets", 7, 0, 7);
    plot1d("h_resttag", values_["resttag"], evtweight_, sr.histMap, ";resolved top tag", 110, -1.1f, 1.1f);
    plot1d("h_bdtttag", values_["bdtttag"], evtweight_, sr.histMap, ";BDT resolved top tag", 110, -1.1f, 1.1f);
    plot1d("h_tfttag", values_["tfttag"], evtweight_, sr.histMap, ";TF resolved top tag", 120, -0.1f, 1.1f);
    plot1d("h_deepttag", values_["deepttag"], evtweight_, sr.histMap, ";deepAK8 top tag", 120, -0.1f, 1.1f);
    plot1d("h_binttag", values_["binttag"], evtweight_, sr.histMap, ";deepAK8 binarized top disc", 120, -0.1f, 1.1f);
    plot1d("h_deepWtag", values_["deepWtag"], evtweight_, sr.histMap, ";deepAK8 W tag", 120, -0.1f, 1.1f);
    plot1d("h_binWtag", values_["binWtag"], evtweight_, sr.histMap, ";deepAK8 binarized W disc", 120, -0.1f, 1.1f);
    plot1d("h_ntftops", values_["ntftops"], evtweight_, sr.histMap, ";ntops from TF tagger", 4, 0, 4);

    plot1d("h_binttag_finedisc", values_["binttag"], evtweight_, sr.histMap, ";deepAK8 binarized top disc", 600, -0.1f, 1.1f);

    float chi2_disc = -log(hadronic_top_chi2()) / 8;
    if (fabs(chi2_disc) >= 1.0) chi2_disc = std::copysign(0.99999, chi2_disc);
    plot1d("h_chi2_disc"+s, chi2_disc, evtweight_, sr.histMap, ";hadronic #chi^2 discriminator", 110, -1.1f, 1.1f);
    plot1d("h_chi2_finedisc"+s, chi2_disc, evtweight_, sr.histMap, ";hadronic #chi^2 discriminator", 550, -1.1f, 1.1f);

    float tmod_disc = values_["tmod"] / 15;
    if (fabs(tmod_disc) >= 1.0) tmod_disc = std::copysign(0.99999, tmod_disc);
    plot1d("h_tmod_disc"+s, tmod_disc, evtweight_, sr.histMap, ";t_{mod} discriminator", 110, -1.1f, 1.1f);
    plot1d("h_tmod_finedisc"+s, tmod_disc, evtweight_, sr.histMap, ";t_{mod} discriminator", 550, -1.1f, 1.1f);

    if (values_["njet"] >= 4) {
      float lead_topcand_disc = (topcands_disc().size() > 0)? topcands_disc()[0] : -1.1;
      plot1d("h_leadtopcand_disc"+s, lead_topcand_disc, evtweight_, sr.histMap, ";top discriminator", 110, -1.1f, 1.1f);
      plot1d("h_leadtopcand_finedisc"+s, lead_topcand_disc, evtweight_, sr.histMap, ";top discriminator", 550, -1.1f, 1.1f);
      plot1d("h_leadtftop_disc"+s, lead_tftop_disc, evtweight_, sr.histMap, ";DeepRes disc", 120, -0.1f, 1.1f);
      plot1d("h_leadtftop_finedisc"+s, lead_tftop_disc, evtweight_, sr.histMap, ";DeepRes disc", 600, -0.1f, 1.1f);
      plot1d("h_chi2_disc_ge4j"+s, chi2_disc, evtweight_, sr.histMap, ";hadronic #chi^2 discriminator", 110, -1.1f, 1.1f);
      plot1d("h_chi2_finedisc_ge4j"+s, chi2_disc, evtweight_, sr.histMap, ";hadronic #chi^2 discriminator", 550, -1.1f, 1.1f);

      plot2d("h2d_tmod_leadres", lead_topcand_disc, values_["tmod"], evtweight_, sr.histMap, ";lead topcand disc;t_{mod}", 55, -1.1f, 1.1f, 50, -10, 15);
      plot2d("h2d_tmod_chi2", chi2_disc, values_["tmod"], evtweight_, sr.histMap, ";lead topcand disc;t_{mod}", 55, -1.1f, 1.1f, 50, -10, 15);
      plot2d("h2d_tmod_restag", values_["resttag"], values_["tmod"], evtweight_, sr.histMap, ";lead topcand disc;t_{mod}", 55, -1.1f, 1.1f, 50, -10, 15);
      plot2d("h2d_mlb_restag", values_["resttag"], values_["mlb"], evtweight_, sr.histMap, ";lead topcand disc;M_{lb}", 55, -1.1f, 1.1f, 50, -10, 15);
      plot2d("h2d_dphijmet_restag", values_["resttag"], values_["dphijmet"], evtweight_, sr.histMap, ";lead topcand disc;#Delta#phi(jet,#slash{E}_{T})", 55, -1.1f, 1.1f, 40, 0, 4);
    }
    plot2d("h2d_njets_nak8", ak8pfjets_deepdisc_top().size(), values_["njet"], evtweight_, sr.histMap, ";Number of AK8 jets; Number of AK4 jets", 7, 0, 7, 8, 2, 10);
    plot2d("h2d_tmod_deeptag", values_["deepttag"], values_["tmod"], evtweight_, sr.histMap, ";lead deepdisc top;t_{mod}", 60, -0.1f, 1.1f, 50, -10, 15);
    plot2d("h2d_dphijmet_deeptag", values_["deepttag"], values_["dphijmet"], evtweight_, sr.histMap, ";lead deepdisc top;#Delta#phi(jet,#slash{E}_{T})", 60, -0.1f, 1.1f, 40, 0, 4);
    plot2d("h2d_mlb_deeptag", values_["deepttag"], values_["mlb"], evtweight_, sr.histMap, ";lead deepdisc top;M_{lb}", 60, -0.1f, 1.1f, 50, -10, 15);
  };

  auto checkMassPt = [&](double mstop, double mlsp) { return (mass_stop() == mstop) && (mass_lsp() == mlsp); };

  for (auto& sr : SRVec) {
    if (!sr.PassesSelection(values_)) continue;
    if (is_data() && !PassingHLTriggers()) continue;
    // Plot kinematics histograms
    fillTopTagHists(sr, suffix);
    if (is_fastsim_ && (checkMassPt(1200, 50) || checkMassPt(800, 400)))
      fillTopTagHists(sr, "_"+to_string((int)mass_stop())+"_"+to_string((int)mass_lsp()) + suffix);
  }
  for (auto& sr : CR2lVec) {
    if (!sr.PassesSelection(values_)) continue;
    if (is_data() && !PassingHLTriggers(2)) continue;
    // Plot kinematics histograms
    fillTopTagHists(sr, suffix);
    if (is_fastsim_ && (checkMassPt(1200, 50) || checkMassPt(800, 400)))
      fillTopTagHists(sr, "_"+to_string((int)mass_stop())+"_"+to_string((int)mass_lsp()) + suffix);
  }
  for (auto& sr : CR0bVec) {
    if (!sr.PassesSelection(values_)) continue;
    if (is_data() && !PassingHLTriggers()) continue;
    // Plot kinematics histograms
    fillTopTagHists(sr, suffix);
    if (is_fastsim_ && (checkMassPt(1200, 50) || checkMassPt(800, 400)))
      fillTopTagHists(sr, "_"+to_string((int)mass_stop())+"_"+to_string((int)mass_lsp()) + suffix);
  }

}

void StopLooper::testTopTaggingEffficiency(SR& sr) {
  // Function to test the top tagging efficiencies and miss-tag rate
  // The current tagger only works for hadronically decay tops
  if (!doTopTagging) return;
  // Temporary for consistency accross samples
  if (pfmet() < 100 && pfmet_rl() < 100) return;

  int n4jets = (ngoodbtags() > 0 && ngoodjets() > 3);

  // First need to determine how many gen tops does hadronic decay
  // int nHadDecayTops = 2 - gen_nfromtleps_();  // 2 gentops for sure <-- checked
  int nHadDecayTops = 2;
  int hadronictopidx = -1;
  int leptonictopidx = -1;
  for (size_t l = 0; l < genleps_id().size(); ++l) {
    if (genleps_isLastCopy().at(l) && genleps_isfromt().at(l) && abs(genleps_motherid().at(l)) == 24 && genleps_gmotheridx().at(l) != leptonictopidx) {
      leptonictopidx = genleps_gmotheridx().at(l);
      nHadDecayTops--;
    }
  }

  int ntopcands = topcands_disc().size();
  float lead_disc = (ntopcands > 0)? topcands_disc().at(0) : -1.09; // lead_bdt_disc
  if (runResTopMVA) {
    lead_disc = values_["resttag"];
  }

  int ntftops = 0;
  float lead_tftop_disc = -0.09;
  ntftops = tftops_p4().size();
  for (auto disc : tftops_disc()) {
    if (disc > lead_tftop_disc) lead_tftop_disc = disc;
  }

  // Hadronic chi2 for comparison
  // Define a disc variable that look similar to resolve top discriminator
  float chi2_disc = -log(hadronic_top_chi2()) / 8;
  if (fabs(chi2_disc) >= 1.0) chi2_disc = std::copysign(0.99999, chi2_disc);
  float chi2_disc2 = std::copysign(pow(fabs(chi2_disc), 0.1), chi2_disc);

  float lead_ak8_pt = 0;
  float lead_deepdisc = -0.1;
  float lead_deepdisc_W = -0.1;
  float lead_bindisc = -0.1;
  float lead_bindisc_W = -0.1;
  float lead_tridisc = -0.1;
  float lead_tridisc_W = -0.1;

  for (size_t iak8 = 0; iak8 < ak8pfjets_p4().size(); ++iak8) {
    float pt = ak8pfjets_p4()[iak8].pt();
    float disc_top = ak8pfjets_deepdisc_top().at(iak8);
    float disc_W = ak8pfjets_deepdisc_w().at(iak8);
    float disc_qcd = ak8pfjets_deepdisc_qcd().at(iak8);

    float bindisc = disc_top / (disc_top + disc_qcd);
    float bindisc_W = disc_W / (disc_W + disc_qcd);

    float den_tri = disc_top + disc_W + disc_qcd;
    float tridisc = disc_top / den_tri;
    float tridisc_W = disc_W / den_tri;

    if (pt > lead_ak8_pt) lead_ak8_pt = pt;
    if (disc_top > lead_deepdisc) lead_deepdisc = disc_top;
    if (disc_W > lead_deepdisc_W) lead_deepdisc_W = disc_W;
    if (bindisc > lead_bindisc) lead_bindisc = bindisc;
    if (bindisc_W > lead_bindisc_W) lead_bindisc_W = bindisc_W;
    if (tridisc > lead_tridisc) lead_tridisc = tridisc;
    if (tridisc_W > lead_tridisc_W) lead_tridisc_W = tridisc_W;
  }

  plot1d("h_lead_deepdisc_W", lead_deepdisc_W, evtweight_, sr.histMap, ";lead AK8 deepdisc W", 120, -0.1f, 1.1f);
  plot1d("h_lead_bindisc_W", lead_bindisc_W, evtweight_, sr.histMap, ";lead AK8 bindisc W", 120, -0.1f, 1.1f);
  plot1d("h_lead_tridisc_W", lead_tridisc_W, evtweight_, sr.histMap, ";lead AK8 bindisc W", 120, -0.1f, 1.1f);

  // Divide the events into tt->1l and tt->2l
  if (nHadDecayTops == 1) {
    // Locate leptonic top, will be used to find the hadronic top
    // int leptonictopidx = -1;
    // for (size_t l = 0; l < genleps_id().size(); ++l) {
    //   if (genleps_isfromt().at(l) && abs(genleps_motherid().at(l)) == 24) {
    //     leptonictopidx = genleps_gmotheridx().at(l);
    //     break;
    //   }
    // }
    // Find the daughters of the hadronically decayed top
    vector<int> jets_fromhadtop;
    vector<int> genq_fromhadtop;
    vector<int> ak8s_fromhadtop;
    float gentop_pt = 0.;
    int bjetidx = -1;
    int topak8idx = -1;
    for (size_t q = 0; q < genqs_id().size(); ++q) {
      if (!genqs_isLastCopy().at(q)) continue;
      if (abs(genqs_id()[q]) == 6 && genqs__genpsidx().at(q) != leptonictopidx && abs(genqs__genpsidx().at(q)-leptonictopidx) == 1) {
        // Found the gen top that decay hadronically
        hadronictopidx = genqs__genpsidx().at(q);
        gentop_pt = genqs_p4().at(q).pt();
        float minDR = 0.8;
        for (size_t j = 0; j < ak8pfjets_p4().size(); ++j) {
          float dr = ROOT::Math::VectorUtil::DeltaR(ak8pfjets_p4().at(j), genqs_p4().at(q));
          if (dr < minDR) {
            minDR = dr;
            topak8idx = j;
          }
        }
      }
      if (!genqs_isfromt().at(q)) continue;
      if ((abs(genqs_motherid().at(q)) == 6 && genqs_motheridx().at(q) == hadronictopidx) ||
          (abs(genqs_motherid().at(q)) == 24 && genqs_gmotheridx().at(q) == hadronictopidx)) {
        genq_fromhadtop.push_back(q);
        if (topak8idx >= 0 && !isCloseObject(ak8pfjets_p4().at(topak8idx), genqs_p4().at(q), 0.8)) {
          topak8idx = -2;
        }
      }
    }

    plot1d("h_sanity_ngenqfromtop", genq_fromhadtop.size(), 1, sr.histMap, ";N gen q fromtop", 7, 0, 7);

    if (genq_fromhadtop.size() < 3) return; // rare event that has lost gen particles, skip

    // // Do duplicate removal when we have the duplicate/split of the same particle <-- or do not
    // vector<pair<int,int>> duppair;  // duplicated quark pair
    // if (genq_fromhadtop.size() > 3) {
    //   for (auto q1 = genq_fromhadtop.begin(); q1 != genq_fromhadtop.end(); ++q1) {
    //     for (auto q2 = q1+1; q2 != genq_fromhadtop.end(); ++q2) {
    //       if (genqs_id().at(*q1) == genqs_id().at(*q2) && genqs_motheridx().at(*q1) == genqs_motheridx().at(*q2))
    //         duppair.emplace_back(*q1, *q2);
    //     }
    //   }
    // }

    // Sort genq_fromhadtop first to put the b quark from top at first
    std::sort(genq_fromhadtop.begin(), genq_fromhadtop.end(), [&](int q1, int q2) { return abs(genqs_motherid().at(q1)) == 6; });

    vector<pair<int,int>> jet_quark_pair;  // jet-quark pair
    long matchedjetidx = 0;                // for fast duplicate check
    for (int q : genq_fromhadtop) {
      int genqid = genqs_id().at(q);
      int jetidx = -1;
      float minDR = 0.6;
      for (size_t j = 0; j < ak4pfjets_p4().size(); ++j) {
        if (ak4pfjets_parton_flavor().at(j) != genqid) continue;
        if (float dr = 1; isCloseObject(ak4pfjets_p4().at(j), genqs_p4().at(q), minDR, &dr)) {
          minDR = dr;
          jetidx = j;
        }
      }
      if (minDR < 0.6 && !(matchedjetidx & 1<<jetidx)) {
        matchedjetidx |= 1<<jetidx;
        jets_fromhadtop.push_back(jetidx);
        if (abs(genqid) == 5 && abs(genqs_motherid().at(q)) == 6) bjetidx = jetidx;
        jet_quark_pair.emplace_back(jetidx, q);
      }
    }

    plot1d("h_njets_fromtop", jets_fromhadtop.size(), 1, sr.histMap, ";N jets fromtop", 7, 0, 7);

    if (jets_fromhadtop.size() < 3) {
      for (auto q : genq_fromhadtop) {
        bool matched = false;
        for (auto jq : jet_quark_pair) {
          if (q == jq.second) {
            matched = true; break;
          }
        }
        if (!matched) {
          plot1d("h_lostjet_genq_pt", genqs_p4().at(q).pt(), evtweight_, sr.histMap, ";p_{T}(gen q from top)", 50, 0, 200);
          plot1d("h_lostjet_genq_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen q from top)", 50, -5, 5);
          if (fabs(genqs_p4().at(q).eta()) < 2.4)
            plot1d("h_lostjet_ineta_genq_pt", genqs_p4().at(q).pt(), evtweight_, sr.histMap, ";p_{T}(gen q from top)", 50, 0, 200);
          if (genqs_p4().at(q).pt() > 30)
            plot1d("h_lostjet_inpt_genq_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen q from top)", 50, -5, 5);

          float minDR_genqs = 7;
          for (auto q2 : genq_fromhadtop) {
            if (float dr = 7; q != q2 && isCloseObject(genqs_p4().at(q), genqs_p4().at(q2), minDR_genqs, &dr))
              minDR_genqs = dr;
          }
          plot1d("h_lostjet_genq_minDR", minDR_genqs, evtweight_, sr.histMap, ";#DeltaR(gen quarks)", 50, 0, 5);
        } else {
          plot1d("h_acptjet_genq_pt", genqs_p4().at(q).pt(), evtweight_, sr.histMap, ";p_{T}(gen q from top)", 50, 0, 200);
          plot1d("h_acptjet_genq_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen q from top)", 50, -5, 5);
        }
      }
    }

    if (jets_fromhadtop.size() == 3) {
      TopCand tc(jets_fromhadtop[0], jets_fromhadtop[1], jets_fromhadtop[2], &ak4pfjets_p4());
      plot1d("h_truecand_topcandpt", tc.topcand.pt(), evtweight_, sr.histMap, ";p_{T}(true topcand)", 60, 0, 600);
      plot1d("h_truecand_topmass", tc.topcand.mass(), evtweight_, sr.histMap, ";Mass(true topcand)", 60, 0, 300);
      plot1d("h_truecand_Wmass", tc.wcand.mass(), evtweight_, sr.histMap, ";Mass(true Wcand)", 60, 0, 300);
      int cat = tc.passMassW() + 2*tc.passMassTop();
      plot1d("h_truecand_cat", cat, evtweight_, sr.histMap, ";category", 5, 0, 5);
    }

    for (int q : genq_fromhadtop) {
      plot1d("h_genqfromtop_pt", genqs_p4().at(q).pt(), evtweight_, sr.histMap, ";p_{T}(gen q from top)", 100, 0, 500);
      plot1d("h_genqfromtop_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen q from top)", 100, -5, 5);
      if (abs(genqs_id().at(q)) == 5) {
        plot1d("h_genbfromtop_pt", genqs_p4().at(q).pt(), evtweight_, sr.histMap, ";p_{T}(gen b from top)", 100, 0, 500);
        plot1d("h_genbfromtop_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen b from top)", 100, -5, 5);
        if (bjetidx < 0) {
          plot1d("h_genblost_eta", genqs_p4().at(q).eta(), evtweight_, sr.histMap, ";#eta(gen b from top)", 100, -5, 5);
        }
      }
    }

    plot1d("h_genmatch_ak8idx_top", topak8idx, evtweight_, sr.histMap, ";ak8jet idx", 7, -2, 5);

    plot1d("hden_gentop_pt", gentop_pt, evtweight_, sr.histMap, ";p_{T}(gen top)", 100, 0, 1500);

    if (topak8idx < 0)
      plot1d("h_mergecat", 0, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
    else if (gentop_pt < 400)
      plot1d("h_mergecat", 1, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
    else if (ak8pfjets_deepdisc_top().at(topak8idx) != lead_deepdisc)
      plot1d("h_mergecat", 2, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);

    // Test plots for the merged tagger, set base to have at least one ak8jet
    if (lead_ak8_pt > 400) {
      plot1d("h_nak8jets_1hadtop", ak8pfjets_p4().size(), evtweight_, sr.histMap, ";Number of AK8 jets", 7, 0, 7);
      plot1d("h_lead_deepdisc_top", lead_deepdisc, evtweight_, sr.histMap, ";lead AK8 deepdisc top", 120, -0.1, 1.1);
      plot1d("h_lead_bindisc_top", lead_bindisc, evtweight_, sr.histMap, ";lead AK8 bindisc top", 120, -0.1, 1.1);
      plot1d("h_lead_tridisc_top", lead_tridisc, evtweight_, sr.histMap, ";lead AK8 tridisc top", 120, -0.1, 1.1);

      plot2d("h2d_lead_deepdisc_topW", lead_deepdisc, lead_deepdisc_W, evtweight_, sr.histMap, ";lead AK8 deepdisc top; lead AK8 deepdisc W", 120, -0.1, 1.1, 120, -0.1, 1.1);
      plot2d("h2d_lead_bindisc_topW", lead_bindisc, lead_bindisc_W, evtweight_, sr.histMap, ";lead AK8 bindisc top; lead AK8 bindisc W", 120, -0.1, 1.1, 120, -0.1, 1.1);
    }

    if (topak8idx >= 0) {
      float truetop_deepdisc = ak8pfjets_deepdisc_top().at(topak8idx);
      float truetop_bindisc = truetop_deepdisc / (truetop_deepdisc + ak8pfjets_deepdisc_qcd().at(topak8idx));
      float truetop_tridisc = truetop_deepdisc / (truetop_deepdisc + ak8pfjets_deepdisc_qcd().at(topak8idx) + ak8pfjets_deepdisc_w().at(topak8idx));
      plot1d("h_truetop_deepdisc_top", truetop_deepdisc, evtweight_, sr.histMap, ";truth-matched AK8 deepdisc top", 120, -0.1, 1.1);
      plot1d("h_truetop_bindisc_top", truetop_bindisc, evtweight_, sr.histMap, ";truth-matched AK8 bindisc top", 120, -0.1, 1.1);
      plot1d("h_truetop_tridisc_top", truetop_tridisc, evtweight_, sr.histMap, ";truth-matched AK8 tridisc top", 120, -0.1, 1.1);

      plot1d("hnum_gentop_pt", gentop_pt, evtweight_, sr.histMap, ";p_{T}(gen top)", 100, 0, 1500);
      plot2d("h2d_ak8_vs_gentop_pt", gentop_pt, ak8pfjets_p4().at(topak8idx).pt(), evtweight_, sr.histMap, ";p_{T}(gen top);p_{T}(ak8jet matched)", 100, 0, 1500, 100, 0, 1500);
      if (truetop_deepdisc > 0.6)
        plot1d("h_truetop_genpt", gentop_pt, evtweight_, sr.histMap, ";p_{T}(gen top)", 100, 0, 1500);

      float truetop_deepdisc_W = ak8pfjets_deepdisc_w().at(topak8idx);
      float truetop_bindisc_W = truetop_deepdisc_W / (truetop_deepdisc_W + ak8pfjets_deepdisc_qcd().at(topak8idx));
      float truetop_tridisc_W = truetop_deepdisc_W / (truetop_deepdisc_W + ak8pfjets_deepdisc_qcd().at(topak8idx) + truetop_deepdisc);
      plot1d("h_truetop_deepdisc_W", truetop_deepdisc_W, evtweight_, sr.histMap, ";truth-matched to top AK8 deepdisc W", 120, -0.1, 1.1);
      plot1d("h_truetop_bindisc_W", truetop_bindisc_W, evtweight_, sr.histMap, ";truth-matched to top AK8 bindisc W", 120, -0.1, 1.1);
      plot1d("h_truetop_tridisc_W", truetop_tridisc_W, evtweight_, sr.histMap, ";truth-matched to top AK8 tridisc W", 120, -0.1, 1.1);

      for (size_t j = 0; j < ak8pfjets_p4().size(); ++j) {
        if ((int)j == topak8idx) continue;
        plot1d("h_nottop_deepdisc_top", ak8pfjets_deepdisc_top().at(j), evtweight_, sr.histMap, ";AK8 not from top, deepdisc top", 120, -0.1, 1.1);
        float nottop_bindisc_top = ak8pfjets_deepdisc_top().at(j)/(ak8pfjets_deepdisc_top().at(j)+ak8pfjets_deepdisc_qcd().at(j));
        plot1d("h_nottop_bindisc_top", nottop_bindisc_top, evtweight_, sr.histMap, ";AK8 not from top, deepdisc top", 120, -0.1, 1.1);
      }

    } else if (topak8idx == -2) {
      plot1d("h_missedsubjet_deepdisc", lead_deepdisc, evtweight_, sr.histMap, ";lead AK8 deepdisc top", 120, -0.1, 1.1);
    } else {
      plot1d("h_missed_deepdisc", lead_deepdisc, evtweight_, sr.histMap, ";lead AK8 deepdisc top", 120, -0.1, 1.1);
    }

    // Test plots for the resolved tagger
    if (n4jets == 0) return;
    plot1d("h_n4jets", n4jets, evtweight_, sr.histMap, ";N(4j events)", 4, 0, 4);

    plot1d("h_chi2_disc1", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
    plot1d("h_chi2_disc2", chi2_disc2, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);

    plot1d("h_ntopcand_raw", ntopcands, evtweight_, sr.histMap, ";N(topcand)", 4, 0, 4);

    plot1d("h_lead_topcand_disc", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 110, -1.1, 1.1);
    plot1d("h_lead_topcand_finedisc", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);

    plot1d("h_ntftops", ntftops, evtweight_, sr.histMap, ";ntops from TF tagger", 4, 0, 4);
    plot1d("h_lead_tftop_disc", lead_tftop_disc, evtweight_, sr.histMap, ";lead TFtop disc", 120, -0.1, 1.1);
    plot1d("h_lead_tftop_finedisc", lead_tftop_disc, evtweight_, sr.histMap, ";lead TFtop finedisc", 1200, -0.1, 1.1);

    if (mt_met_lep() > 150) {
      plot1d("h_chi2_disc1_mt150", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
      plot1d("h_lead_topcand_finedisc_mt150", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);
      plot1d("h_lead_tftop_finedisc_mt150", lead_tftop_disc, evtweight_, sr.histMap, ";ntops from TF tagger", 1200, -0.1, 1.1);
    }

    if (mindphi_met_j1_j2() > 0.5) {
      plot1d("h_chi2_disc1_dphigp5", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
      plot1d("h_lead_topcand_finedisc_dphigp5", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);
      plot1d("h_lead_tftop_finedisc_dphigp5", lead_tftop_disc, evtweight_, sr.histMap, ";ntops from TF tagger", 1200, -0.1, 1.1);
    }

    for (float disc : topcands_disc()) {
      plot1d("h_all_topcand_disc", disc, evtweight_, sr.histMap, ";discriminator", 110, -1.1, 1.1);
      plot1d("h_all_topcand_finedisc", disc, evtweight_, sr.histMap, ";discriminator", 1100, -1.1, 1.1);
    }

    if (jets_fromhadtop.size() < 3) {
      // Check all jets from top decay are within the list
      if (lead_disc < 0.98)
        plot1d("h_category", 0, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
      else
        plot1d("h_category", 1, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
    } else if (ntopcands < 1) {
      plot1d("h_category", 7, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
    } else {
      if (bjetidx >= 0) {
        plot1d("h_bfromt_csv", ak4pfjets_CSV().at(bjetidx), evtweight_, sr.histMap, ";CSVv2 (gen b)", 100, 0, 1);
        plot1d("h_bfromt_deepcsv", ak4pfjets_deepCSV().at(bjetidx), evtweight_, sr.histMap, ";CSVv2 (gen b)", 100, 0, 1);
      }
      plot1d("h_allaccepted_disc", topcands_disc().at(0), evtweight_, sr.histMap, ";lead topcand discriminator", 110, -1.1, 1.1);

      vector<int> jidxs = topcands_ak4idx().at(0);
      bool isRealTop = std::is_permutation(jidxs.begin(), jidxs.end(), jets_fromhadtop.begin());
      if (lead_disc < 0.98) {
        if (jidxs.at(0) != bjetidx)
          plot1d("h_category", 2, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
        else if (isRealTop)
          plot1d("h_category", 3, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
        else
          plot1d("h_category", 4, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
      } else {
        if (!isRealTop)
          plot1d("h_category", 5, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
        else
          plot1d("h_category", 6, evtweight_, sr.histMap, ";category for toptagging", 8, 0, 8);
      }
    }

    if (ntopcands >= 1) {
      bool isActualTopJet = true;
      float gentoppt = 0.;
      vector<int> jidxs = topcands_ak4idx().at(0);
      vector<int> midxs;
      for (int j = 0; j < 3; ++j) {
        int nMatchedGenqs = 0;
        int genqidx = -1;
        float minDR = 0.4;
        for (size_t i = 0; i < genqs_id().size(); ++i) {
          if (genqs_status().at(i) != 23 && genqs_status().at(i) != 1) continue;
          float dr = ROOT::Math::VectorUtil::DeltaR(ak4pfjets_p4().at(jidxs[j]), genqs_p4().at(i));
          if (dr > minDR) continue;
          nMatchedGenqs++;
          genqidx = i;
          minDR = dr;
        }
        if (lead_disc >= 0.98)
          plot1d("h_nMatchedGenqs", nMatchedGenqs, evtweight_, sr.histMap, ";nMatchedGenqs", 4, 0, 4);
        if (nMatchedGenqs < 1) {
          isActualTopJet = false;
          break;
        }
        if (lead_disc >= 0.98)
          plot1d("h_jqminDR", minDR, evtweight_, sr.histMap, ";min(#Delta R)", 41, 0, 0.41);
        if (!genqs_isfromt().at(genqidx)) {
          isActualTopJet = false;
          break;
        }
        if (abs(genqs_id().at(genqidx)) == 5 && abs(genqs_motherid().at(genqidx)) == 6) {
          gentoppt = genqs_motherp4().at(genqidx).pt();
          if (leptonictopidx > 0 && leptonictopidx != genqs_motheridx().at(genqidx)) {
            midxs.push_back(genqidx);
          } else {
            isActualTopJet = false;
            break;
          }
        }
        else if (abs(genqs_motherid().at(genqidx)) == 24) {
          midxs.push_back(genqidx);
        }
      }
      if (midxs.size() != 3) isActualTopJet = false;

      plot1d("hden_disc", topcands_disc().at(0), evtweight_, sr.histMap, ";discriminator", 110, -1.1, 1.1);
      if (lead_disc >= 0.98) {
        plot1d("hden_pt", topcands_p4().at(0).pt(), evtweight_, sr.histMap, ";discriminator", 110, 0, 1100);
        plot1d("hden_gentoppt", gentoppt, evtweight_, sr.histMap, ";p_{T}(gen top)", 110, 0, 1100);
      }
      if (isActualTopJet) {
        plot1d("hnum_disc", topcands_disc().at(0), evtweight_, sr.histMap, ";topcand discriminator", 110, -1.1, 1.1);
        if (lead_disc >= 0.98) {
          plot1d("hnum_pt", topcands_p4().at(0).pt(), evtweight_, sr.histMap, ";topcand pt", 110, 0, 1100);
          plot1d("hnum_gentoppt", gentoppt, evtweight_, sr.histMap, ";p_{T}(gen top)", 110, 0, 1100);
        }
      }
    }
  }
  else if (nHadDecayTops == 0) {

    if (lead_ak8_pt > 400) {
      plot1d("h_nak8jets_0hadtop", ak8pfjets_p4().size(), evtweight_, sr.histMap, ";Number of AK8 jets", 7, 0, 7);
      plot1d("h_lead_deepdisc_faketop", lead_deepdisc, evtweight_, sr.histMap, ";lead AK8 deepdisc top", 120, -0.1, 1.1);
      plot1d("h_lead_bindisc_faketop", lead_bindisc, evtweight_, sr.histMap, ";lead AK8 bindisc top", 120, -0.1, 1.1);
      plot1d("h_lead_tridisc_faketop", lead_tridisc, evtweight_, sr.histMap, ";lead AK8 tridisc top", 120, -0.1, 1.1);
    }

    if (n4jets == 0) return;
    plot1d("h_fak4j", n4jets, evtweight_, sr.histMap, ";N(events)", 4, 0, 4);

    plot1d("h_chi2fake_disc1", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
    plot1d("h_chi2fake_disc2", chi2_disc2, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);

    plot1d("h_nfakecand_raw", ntopcands, evtweight_, sr.histMap, ";N(topcand)", 4, 0, 4);

    plot1d("h_lead_fakecand_disc", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 110, -1.1, 1.1);
    plot1d("h_lead_fakecand_finedisc", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);

    plot1d("h_nfaketftops", ntftops, evtweight_, sr.histMap, ";ntops from TF tagger", 4, 0, 4);
    plot1d("h_lead_faketftop_disc", lead_tftop_disc, evtweight_, sr.histMap, ";ntops from TF tagger", 120, -0.1, 1.1);
    plot1d("h_lead_faketftop_finedisc", lead_tftop_disc, evtweight_, sr.histMap, ";lead TFtop disc", 1200, -0.1, 1.1);

    if (mt_met_lep() > 150) {
      plot1d("h_chi2fake_disc1_mt150", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
      plot1d("h_lead_fakecand_finedisc_mt150", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);
      plot1d("h_lead_faketftop_finedisc_mt150", lead_tftop_disc, evtweight_, sr.histMap, ";lead TFtop disc", 1200, -0.1, 1.1);
    }

    if (mindphi_met_j1_j2() > 0.5) {
      plot1d("h_chi2fake_disc1_dphigp5", chi2_disc, evtweight_, sr.histMap, ";discriminator", 220, -1.1, 1.1);
      plot1d("h_lead_fakecand_finedisc_dphigp5", lead_disc, evtweight_, sr.histMap, ";lead topcand discriminator", 1100, -1.1, 1.1);
      plot1d("h_lead_faketftop_finedisc_dphigp5", lead_tftop_disc, evtweight_, sr.histMap, ";lead TFtop disc", 1200, -0.1, 1.1);
    }

    plot1d("hden_fakep5_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
    plot1d("hden_fakep9_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
    plot1d("hden_fakep98_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
    for (float disc : topcands_disc()) {
      plot1d("h_fakecand_disc", disc, evtweight_, sr.histMap, ";discriminator", 110, -1.1, 1.1);
      plot1d("h_fakecand_finedisc", disc, evtweight_, sr.histMap, ";discriminator", 1100, -1.1, 1.1);
    }
    if (lead_disc > 0.98)
      plot1d("hnum_fakep98_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
    if (lead_disc > 0.9)
      plot1d("hnum_fakep9_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
    if (lead_disc > 0.5)
      plot1d("hnum_fakep5_pt", lead_disc, evtweight_, sr.histMap, ";fakecand pt", 110, 0, 1100);
  }
}

void StopLooper::testCutFlowHistos(SR& sr) {

  auto fillCFhist = [&](string&& hname, auto& cutflow, auto& cfvallow, auto& cfvalupp, auto& cfnames) {
    int ncuts = cutflow.size();
    if (sr.histMap.count(hname) == 0) {
      plot1d(hname, 0, 1, sr.histMap, "cutflow;icut", ncuts+1, 0, ncuts+1);
      sr.histMap[hname]->GetXaxis()->SetBinLabel(1, "base");
      for (int icf = 0; icf < ncuts; ++icf)
        sr.histMap[hname]->GetXaxis()->SetBinLabel(icf+2, cfnames[icf].c_str());
    } else {
      plot1d(hname, 0, 1, sr.histMap, "cutflow;icut", ncuts+1, 0, ncuts+1);
    }
    for (int icf = 0; icf < ncuts; ++icf) {
      string cut = cutflow[icf];
      if (values_[cut] >= cfvallow[icf] && values_[cut] < cfvalupp[icf]) {
        plot1d(hname, icf+1, 1, sr.histMap, "cutflow;icut", ncuts+1, 0, ncuts+1);
      } else {
        break;
      }
    }
  };

  string masspt_suf = (is_fastsim_)? "_" + to_string((int) mass_stop()) + "_" + to_string((int) mass_lsp()) : "";

  const vector<string> cfnames1 = {"met",  "mt",};
  const vector<string> cutflow1 = {"met",  "mt",};
  const vector<float> cfvallow1 = {  250,  100 ,};
  const vector<float> cfvalupp1 = { fInf, fInf ,};
  fillCFhist("h_cutflow1_org", cutflow1, cfvallow1, cfvalupp1, cfnames1);
  if (is_fastsim_) fillCFhist("h_cutflow1"+masspt_suf+"_org", cutflow1, cfvallow1, cfvalupp1, cfnames1);
  if (doTopTagging && topcands_disc().size() > 0 && topcands_disc()[0] > 0.98) {
    fillCFhist("h_cutflow1_wtc", cutflow1, cfvallow1, cfvalupp1, cfnames1);
    if (is_fastsim_) fillCFhist("h_cutflow1"+masspt_suf+"c", cutflow1, cfvallow1, cfvalupp1, cfnames1);
  }

  const vector<string> cfnames2 = {  "mt",  "mlb",};
  const vector<string> cutflow2 = {  "mt",  "mlb",};
  const vector<float> cfvallow2 = {  150 ,   100 ,};
  const vector<float> cfvalupp2 = { fInf ,  fInf ,};
  fillCFhist("h_cutflow2_org", cutflow2, cfvallow2, cfvalupp2, cfnames2);
  if (is_fastsim_) fillCFhist("h_cutflow2"+masspt_suf+"_org", cutflow2, cfvallow2, cfvalupp2, cfnames2);
  if (doTopTagging && topcands_disc().size() > 0 && topcands_disc()[0] > 0.98) {
    fillCFhist("h_cutflow2_wtc", cutflow2, cfvallow2, cfvalupp2, cfnames2);
    if (is_fastsim_) fillCFhist("h_cutflow2"+masspt_suf+"_wtc", cutflow2, cfvallow2, cfvalupp2, cfnames2);
  }

  const vector<string> cfnames3 = { "njet4", "njet5",};
  const vector<string> cutflow3 = {  "njet",  "njet",};
  const vector<float> cfvallow3 = {      4 ,      5 ,};
  const vector<float> cfvalupp3 = {   fInf ,   fInf ,};
  fillCFhist("h_cutflow3_org", cutflow3, cfvallow3, cfvalupp3, cfnames3);
  if (is_fastsim_) fillCFhist("h_cutflow3"+masspt_suf+"_org", cutflow3, cfvallow3, cfvalupp3, cfnames3);
  if (doTopTagging && topcands_disc().size() > 0 && topcands_disc()[0] > 0.98) {
    fillCFhist("h_cutflow3_wtc", cutflow3, cfvallow3, cfvalupp3, cfnames3);
    if (is_fastsim_) fillCFhist("h_cutflow3"+masspt_suf+"_wtc", cutflow3, cfvallow3, cfvalupp3, cfnames3);
  }

}
