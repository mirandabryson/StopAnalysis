#include <string.h>
#include <iostream>
#include <vector>
#include <typeinfo>
#include <cmath>
#include <utility>

#include "looper.h"

#include "TBenchmark.h"
#include "TChain.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TROOT.h"
#include "TTreeCache.h"
#include "TGraphAsymmErrors.h"

#include "CMS3.h"
#include "MCSelections.h"
#include "StopSelections.h"
#include "TriggerSelections.h"
#include "stop_variables/mt2w.h"
#include "stop_variables/mt2w_bisect.h"
#include "stop_variables/chi2.h"
#include "stop_variables/JetUtil.h"
#include "stop_variables/topness.h"
#include "stop_variables/MT2_implementations.h"
#include "JetCorrector.h"
//#include "btagsf/BTagCalibrationStandalone.h"

// CORE
#include "Config.h"
#include "PhotonSelections.h"
#include "MuonSelections.h"
#include "IsolationTools.h"
#include "MetSelections.h"

// StopCORE
#include "../StopCORE/eventWeight_lepSF.h"

// CORE/Tools
#include "goodrun.h"
#include "utils.h"
#include "dorky/dorky.h"
#include "datasetinfo/getDatasetInfo.h"


typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float> > LorentzVector;

using namespace std;
using namespace tas;
using namespace duplicate_removal;


//====================//
//                    //
// Utility Structures //
//                    //
//====================//

struct Lepton{
  int id;
  int idx;
  LorentzVector p4;
  //Lepton(id, idx, p4) {id = id; idx = idx; p4 = p4;}
};

struct sortbypt{
  bool operator () (const pair<int, LorentzVector> &v1, const pair<int,LorentzVector> &v2){
    return v1.second.pt() > v2.second.pt();
  }
};

struct sortLepbypt{
  bool operator () (const Lepton &lep1, const Lepton &lep2){
    return lep1.p4.pt() > lep2.p4.pt();
  }
};

struct sortP4byPt {
  bool operator () (const LorentzVector &lv1, const LorentzVector &lv2) { return lv1.pt() > lv2.pt(); }
};

bool CompareIndexValueGreatest(const std::pair<double, int>& firstElem, const std::pair<double, int>& secondElem) {
  return firstElem.first > secondElem.first;
}
bool CompareIndexValueSmallest(const std::pair<double, int>& firstElem, const std::pair<double, int>& secondElem) {
  return firstElem.first < secondElem.first;
}


//===========//
//           //
// Functions //
//           //
//===========//

babyMaker::babyMaker() :
    StopEvt(),
    lep1("lep1_"),
    lep2("lep2_"),
    ph("ph_"),
    jets(),
    jets_jup("jup_"),
    jets_jdown("jdown_"),
    Taus(),
    Tracks(),
    gen_leps("leps_"),
    gen_nus("nus_"),
    gen_qs ("qs_"),
    gen_bosons("bosons_"),
    gen_susy("susy_")
{
  //obsolete
  //gen_tops = GenParticleTree("ts_");  //merged into gen_qs
  //gen_els = GenParticleTree("els_");
  //gen_mus = GenParticleTree("mus_");
  //gen_taus = GenParticleTree("taus_");
  //gen_nuels = GenParticleTree("nuels_");
  //gen_numus = GenParticleTree("numus_");
  //gen_nutaus = GenParticleTree("nutaus_");
  //gen_bs  = GenParticleTree("bs_");
  //gen_cs  = GenParticleTree("cs_");
  //gen_glus = GenParticleTree("glus_");
  //gen_ws  = GenParticleTree("ws_");
  //gen_zs  = GenParticleTree("zs_");
  //gen_phs = GenParticleTree("phs_");
  //gen_hs  = GenParticleTree("hs_");
}

void babyMaker::MakeBabyNtuple(const char* output_name){

  BabyFile = new TFile(Form("%s/%s", babypath, output_name), "RECREATE");
  BabyTree = new TTree("t", "Stop2017 Baby Ntuple");

  StopEvt.SetBranches(BabyTree);
  lep1.SetBranches(BabyTree);
  lep2.SetBranches(BabyTree);
  ph.SetBranches(BabyTree);
  jets.SetAK4Branches(BabyTree);
  jets_jup.SetAK4Branches(BabyTree);
  jets_jdown.SetAK4Branches(BabyTree);
  // Taus.SetBranches(BabyTree);
  // Tracks.SetBranches(BabyTree);
  gen_leps.SetBranches(BabyTree);
  gen_nus.SetBranches(BabyTree);
  gen_qs.SetBranches(BabyTree);
  gen_bosons.SetBranches(BabyTree);
  gen_susy.SetBranches(BabyTree);

  //optional
  if(fillAK8){
    jets.SetAK8Branches(BabyTree);
    jets_jup.SetAK8Branches(BabyTree);
    jets_jdown.SetAK8Branches(BabyTree);
  }
  if(fillTopTag){
    jets.SetAK4Branches_TopTag(BabyTree);
    jets_jup.SetAK4Branches_TopTag(BabyTree);
    jets_jdown.SetAK4Branches_TopTag(BabyTree);

    // Setup MVA Reader TopTagging for 
    // ResolvedTopMVA* resTopMVAptr =  new ResolvedTopMVA("ResTopTagger/resTop_xGBoost_v0.weights.xml", "BDT");
    ResolvedTopMVA* resTopMVAptr =  new ResolvedTopMVA("ResTopTagger/resTop_weights_xGBoost_v3.xml", "BDT");
    jets.InitTopMVA(resTopMVAptr);
    jets_jup.InitTopMVA(resTopMVAptr);
    jets_jdown.InitTopMVA(resTopMVAptr);
  }

  if(filltaus)  Taus.SetBranches(BabyTree);
  if(filltracks)  Tracks.SetBranches(BabyTree);

  if(fillZll)  StopEvt.SetZllBranches(BabyTree);
  if(fillPhoton) StopEvt.SetPhotonBranches(BabyTree);
  if(fillMETfilt) StopEvt.SetMETFilterBranches(BabyTree);
  if(fill2ndlep) StopEvt.SetSecondLepBranches(BabyTree);
  if(fillExtraEvtVar) StopEvt.SetExtraVariablesBranches(BabyTree);

  if(fillAK4EF)   jets.SetAK4Branches_EF(BabyTree);
  if(fillAK4_Other)  jets.SetAK4Branches_Other(BabyTree);
  if(fillOverleps)  jets.SetAK4Branches_Overleps(BabyTree);
  if(fillAK4Synch)  jets.SetAK4Branches_SynchTools(BabyTree);
  if(fillElID)  lep1.SetBranches_electronID(BabyTree);
  if(fillIso)  lep1.SetBranches_Iso(BabyTree);
  if(fillLepSynch)  lep1.SetBranches_SynchTools(BabyTree);
  if(fillElID)  lep2.SetBranches_electronID(BabyTree);
  if(fillIso)  lep2.SetBranches_Iso(BabyTree);
  if(fillLepSynch)  lep2.SetBranches_SynchTools(BabyTree);

  //obsolete
  //gen_els.SetBranches(BabyTree);
  //gen_mus.SetBranches(BabyTree);
  //gen_taus.SetBranches(BabyTree);
  //gen_nuels.SetBranches(BabyTree);
  //gen_numus.SetBranches(BabyTree);
  //gen_nutaus.SetBranches(BabyTree);
  //gen_tops.SetBranches(BabyTree);
  //gen_bs.SetBranches(BabyTree);
  //gen_cs.SetBranches(BabyTree);
  //gen_glus.SetBranches(BabyTree);
  //gen_ws.SetBranches(BabyTree);
  //gen_zs.SetBranches(BabyTree);
  //gen_phs.SetBranches(BabyTree);
  //gen_hs.SetBranches(BabyTree);
}

void babyMaker::InitBabyNtuple(){

  StopEvt.Reset();
  lep1.Reset();
  lep2.Reset();
  jets.Reset();
  jets_jup.Reset();
  jets_jdown.Reset();
  ph.Reset();
  Taus.Reset();
  Tracks.Reset();

  gen_leps.Reset();
  gen_nus.Reset();
  gen_qs.Reset();
  gen_bosons.Reset();
  gen_susy.Reset();

  //obsolete
  //gen_els.Reset();
  //gen_mus.Reset();
  //gen_taus.Reset();
  //gen_nuels.Reset();
  //gen_numus.Reset();
  //gen_nutaus.Reset();
  //gen_tops.Reset();
  //gen_bs.Reset();
  //gen_cs.Reset();
  //gen_qs.Reset();
  //gen_glus.Reset();
  //gen_ws.Reset();
  //gen_zs.Reset();
  //gen_phs.Reset();
  //gen_lsp.Reset();
  //gen_stop.Reset();
  //gen_tops.Reset();

}




//================//
//                //
// Main function  //
//                //
//================//

int babyMaker::looper(TChain* chain, char* output_name, int nEvents, char* path){

  //
  // Set output file path
  //
  babypath = path;

  //
  // Benchmark
  //
  TBenchmark *bmark = new TBenchmark();
  bmark->Start("benchmark");

  //
  //Set up loop over chain
  //
  unsigned int nEvents_processed = 0;
  unsigned int nEvents_pass_skim_nVtx = 0;
  unsigned int nEvents_pass_skim_met = 0;
  unsigned int nEvents_pass_skim_met_emuEvt = 0;
  unsigned int nEvents_pass_skim_nGoodLep = 0;
  unsigned int nEvents_pass_skim_nJets = 0;
  unsigned int nEvents_pass_skim_nBJets=0;
  unsigned int nEvents_pass_skim_2ndlepVeto=0;
  unsigned int nEventsToDo = chain->GetEntries();
  unsigned int jet_overlep1_idx = -9999;
  unsigned int jet_overlep2_idx = -9999;

  //unsigned int track_overlep1_idx = -9999;
  //unsigned int track_overlep2_idx = -9999;

  if (nEvents >= 0) nEventsToDo = nEvents;
  TObjArray *listOfFiles = chain->GetListOfFiles();
  TIter fileIter(listOfFiles);
  TFile *currentFile = 0;
  bool applyJECunc = (applyJECfromFile && (JES_type != 0));

  //
  // JEC files
  //
  bool isDataFromFileName;
  bool isSignalFromFileName;
  bool isTChiFromFileName = false;
  string outfilestr(output_name);
  string filestr(chain->GetFile()->GetName());
  cout << "[looper] >> The utput name will be " << output_name << ".root";
  if (filestr.find("data") != std::string::npos) {
    isDataFromFileName = true;
    isSignalFromFileName = false;
    cout << ", running on DATA, based on input file name." << endl;
  }
  else if ((filestr.find("SMS") != std::string::npos) || (outfilestr.find("Signal") != std::string::npos)) {
    isDataFromFileName = false;
    isSignalFromFileName = true;
    cout << ", running on SIGNAL, based on input file name." << endl;
    if (filestr.find("mLSP") != std::string::npos)  // sample is probably fullsim if mLSP is fixed
      isSignalFromFileName = false;
    if (filestr.find("TChi") != std::string::npos)
      isTChiFromFileName = true;
  }
  else {
    isDataFromFileName = false;
    isSignalFromFileName = false;
    cout << ", running on MC, based on input file name." << endl;
  }

  gconf.GetConfigsFromDatasetName(filestr);
  bool isFall17FastExt1 = (filestr.find("Fall17Fast") != string::npos && filestr.find("_ext1-v1") != string::npos);

  // Setup lepton SF histos
  eventWeight_lepSF lepsf;
  const double matched_dr = 0.1;  // match DR between genlep and recolep
  if( (applyLeptonSFs || applyVetoLeptonSFs) && !isDataFromFileName){
    TString lepsf_filepath;
    if (gconf.year == 2016 && gconf.cmssw_ver == 80) {
      lepsf_filepath = "lepsf/analysis2016_36p46fb";
    } else if (gconf.year == 2016) {
      lepsf_filepath = "lepsf/analysisRun2_2016";
    } else if (gconf.year == 2017) {
      lepsf_filepath = "lepsf/analysisRun2_2017";
    } else if (gconf.year == 2018) {
      lepsf_filepath = "lepsf/analysisRun2_2018";
    }

    if (isFall17FastExt1)
      lepsf.setup(isSignalFromFileName, 2018, "lepsf/analysisRun2_2018");
    else
      lepsf.setup(isSignalFromFileName, gconf.year, lepsf_filepath);
  } // end if applying lepton SFs


  TFile *fxsec;
  TH1D *hxsec;
  if(isSignalFromFileName){
    TString xsecFileName = "xsec_susy_13TeV_run2.root";
    if(isTChiFromFileName) xsecFileName = "xsec_tchi_13TeV.root";
    fxsec = new TFile(xsecFileName,"READ");
    if(fxsec->IsZombie()) {
      std::cout << "Somehow xsec root file is corrupted. Exit..." << std::endl;
      exit(0);
    }
    if(!isTChiFromFileName)
      hxsec = (TH1D*)fxsec->Get("h_xsec_stop");
    else
      hxsec = (TH1D*)fxsec->Get("h_xsec_c1n2");
  }
  TFile *pileupfile;
  TH1D *hPU;
  TH1D *hPUup;
  TH1D *hPUdown;
  if(!isDataFromFileName){
    pileupfile = new TFile("puWeights_Run2.root","READ");
    if(pileupfile->IsZombie()) {
      std::cout << "The pileup weight file puWeights_Run2.root is corrupted. Exit..." << std::endl;
      exit(0);
    }
    hPU     = (TH1D*)pileupfile->Get(Form("puWeight%d", gconf.year));
    hPUup   = (TH1D*)pileupfile->Get(Form("puWeight%dUp", gconf.year));
    hPUdown = (TH1D*)pileupfile->Get(Form("puWeight%dDown", gconf.year));
  }

  TH1D* counterhist = new TH1D( "h_counter", "h_counter", 50, 0.5,50.5);
  counterhist->Sumw2();
  counterhist->GetXaxis()->SetBinLabel(1,"nominal,muR=1 muF=1");
  counterhist->GetXaxis()->SetBinLabel(2,"muR=1 muF=2");
  counterhist->GetXaxis()->SetBinLabel(3,"muR=1 muF=0.5");
  counterhist->GetXaxis()->SetBinLabel(4,"muR=2 muF=1");
  counterhist->GetXaxis()->SetBinLabel(5,"muR=2 muF=2");
  counterhist->GetXaxis()->SetBinLabel(6,"muR=2 muF=0.5");
  counterhist->GetXaxis()->SetBinLabel(7,"muR=0.5 muF=1");
  counterhist->GetXaxis()->SetBinLabel(8,"muR=0.5 muF=2");
  counterhist->GetXaxis()->SetBinLabel(9,"muR=0.5 muF=0.5");
  counterhist->GetXaxis()->SetBinLabel(10,"pdf_up");
  counterhist->GetXaxis()->SetBinLabel(11,"pdf_down");
  counterhist->GetXaxis()->SetBinLabel(12,"pdf_alphas_var_1");
  counterhist->GetXaxis()->SetBinLabel(13,"pdf_alphas_var_2");
  counterhist->GetXaxis()->SetBinLabel(14,"weight_btagsf");
  counterhist->GetXaxis()->SetBinLabel(15,"weight_btagsf_heavy_UP");
  counterhist->GetXaxis()->SetBinLabel(16,"weight_btagsf_light_UP");
  counterhist->GetXaxis()->SetBinLabel(17,"weight_btagsf_heavy_DN");
  counterhist->GetXaxis()->SetBinLabel(18,"weight_btagsf_light_DN");
  counterhist->GetXaxis()->SetBinLabel(19,"weight_ISR_nominal");
  counterhist->GetXaxis()->SetBinLabel(20,"weight_ISR_up");
  counterhist->GetXaxis()->SetBinLabel(21,"weight_ISR_down");
  counterhist->GetXaxis()->SetBinLabel(22,"NEvents");
  counterhist->GetXaxis()->SetBinLabel(23,"weight_btagsf_fastsim_UP");
  counterhist->GetXaxis()->SetBinLabel(24,"weight_btagsf_fastsim_DN");
  counterhist->GetXaxis()->SetBinLabel(25,"weight_ISRnjets_nominal");
  counterhist->GetXaxis()->SetBinLabel(26,"weight_ISRnjets_up");
  counterhist->GetXaxis()->SetBinLabel(27,"weight_ISRnjets_down");
  counterhist->GetXaxis()->SetBinLabel(28,"weight_lepSF");
  counterhist->GetXaxis()->SetBinLabel(29,"weight_lepSF_up");
  counterhist->GetXaxis()->SetBinLabel(30,"weight_lepSF_down");
  counterhist->GetXaxis()->SetBinLabel(31,"weight_vetoLepSF");
  counterhist->GetXaxis()->SetBinLabel(32,"weight_vetoLepSF_up");
  counterhist->GetXaxis()->SetBinLabel(33,"weight_vetoLepSF_down");
  counterhist->GetXaxis()->SetBinLabel(34,"weight_lepSF_fastSim");
  counterhist->GetXaxis()->SetBinLabel(35,"weight_lepSF_fastSim_up");
  counterhist->GetXaxis()->SetBinLabel(36,"weight_lepSF_fastSim_down");
  counterhist->GetXaxis()->SetBinLabel(37,"weight_tightbtagsf");
  counterhist->GetXaxis()->SetBinLabel(38,"weight_tightbtagsf_heavy_UP");
  counterhist->GetXaxis()->SetBinLabel(39,"weight_tightbtagsf_light_UP");
  counterhist->GetXaxis()->SetBinLabel(40,"weight_tightbtagsf_heavy_DN");
  counterhist->GetXaxis()->SetBinLabel(41,"weight_tightbtagsf_light_DN");
  counterhist->GetXaxis()->SetBinLabel(42,"weight_tightbtagsf_fastsim_UP");
  counterhist->GetXaxis()->SetBinLabel(43,"weight_tightbtagsf_fastsim_DN");
  counterhist->GetXaxis()->SetBinLabel(44,"weight_loosebtagsf");
  counterhist->GetXaxis()->SetBinLabel(45,"weight_loosebtagsf_heavy_UP");
  counterhist->GetXaxis()->SetBinLabel(46,"weight_loosebtagsf_light_UP");
  counterhist->GetXaxis()->SetBinLabel(47,"weight_loosebtagsf_heavy_DN");
  counterhist->GetXaxis()->SetBinLabel(48,"weight_loosebtagsf_light_DN");
  counterhist->GetXaxis()->SetBinLabel(49,"weight_loosebtagsf_fastsim_UP");
  counterhist->GetXaxis()->SetBinLabel(50,"weight_loosebtagsf_fastsim_DN");

  TH3D* counterhistSig;
  TH2F* histNEvts;//count #evts per signal point
  if(isSignalFromFileName){//create histos only for signals
    //counterhistSig = new TH3D( "h_counterSMS", "h_counterSMS", 305,-1,1524, 205,-1,1024, 50, 0.5,50.5);//3 million bins! - too much
    counterhistSig = new TH3D( "h_counterSMS", "h_counterSMS", 81,-12.5,2012.5, 61,-12.5,1512.5, 50, 0.5,50.5);//250'000 bins!
    counterhistSig->Sumw2();
    counterhistSig->GetZaxis()->SetBinLabel(1,"nominal,muR=1 muF=1");
    counterhistSig->GetZaxis()->SetBinLabel(2,"muR=1 muF=2");
    counterhistSig->GetZaxis()->SetBinLabel(3,"muR=1 muF=0.5");
    counterhistSig->GetZaxis()->SetBinLabel(4,"muR=2 muF=1");
    counterhistSig->GetZaxis()->SetBinLabel(5,"muR=2 muF=2");
    counterhistSig->GetZaxis()->SetBinLabel(6,"muR=2 muF=0.5");
    counterhistSig->GetZaxis()->SetBinLabel(7,"muR=0.5 muF=1");
    counterhistSig->GetZaxis()->SetBinLabel(8,"muR=0.5 muF=2");
    counterhistSig->GetZaxis()->SetBinLabel(9,"muR=0.5 muF=0.5");
    counterhistSig->GetZaxis()->SetBinLabel(10,"pdf_up");
    counterhistSig->GetZaxis()->SetBinLabel(11,"pdf_down");
    counterhistSig->GetZaxis()->SetBinLabel(12,"pdf_alphas_var_1");
    counterhistSig->GetZaxis()->SetBinLabel(13,"pdf_alphas_var_2");
    counterhistSig->GetZaxis()->SetBinLabel(14,"weight_btagsf");
    counterhistSig->GetZaxis()->SetBinLabel(15,"weight_btagsf_heavy_UP");
    counterhistSig->GetZaxis()->SetBinLabel(16,"weight_btagsf_light_UP");
    counterhistSig->GetZaxis()->SetBinLabel(17,"weight_btagsf_heavy_DN");
    counterhistSig->GetZaxis()->SetBinLabel(18,"weight_btagsf_light_DN");
    counterhistSig->GetZaxis()->SetBinLabel(19,"weight_ISR_nominal");
    counterhistSig->GetZaxis()->SetBinLabel(20,"weight_ISR_up");
    counterhistSig->GetZaxis()->SetBinLabel(21,"weight_ISR_down");
    counterhistSig->GetZaxis()->SetBinLabel(22,"weight_btagsf_fastsim_UP");
    counterhistSig->GetZaxis()->SetBinLabel(23,"weight_btagsf_fastsim_DN");
    counterhistSig->GetZaxis()->SetBinLabel(24,"weight_ISRnjets_nominal");
    counterhistSig->GetZaxis()->SetBinLabel(25,"weight_ISRnjets_up");
    counterhistSig->GetZaxis()->SetBinLabel(26,"weight_ISRnjets_down");
    counterhistSig->GetZaxis()->SetBinLabel(27,"weight_lepSF");
    counterhistSig->GetZaxis()->SetBinLabel(28,"weight_lepSF_up");
    counterhistSig->GetZaxis()->SetBinLabel(29,"weight_lepSF_down");
    counterhistSig->GetZaxis()->SetBinLabel(30,"weight_vetoLepSF");
    counterhistSig->GetZaxis()->SetBinLabel(31,"weight_vetoLepSF_up");
    counterhistSig->GetZaxis()->SetBinLabel(32,"weight_vetoLepSF_down");
    counterhistSig->GetZaxis()->SetBinLabel(33,"weight_lepSF_fastSim");
    counterhistSig->GetZaxis()->SetBinLabel(34,"weight_lepSF_fastSim_up");
    counterhistSig->GetZaxis()->SetBinLabel(35,"weight_lepSF_fastSim_down");
    counterhistSig->GetZaxis()->SetBinLabel(36,"nevents");
    counterhistSig->GetZaxis()->SetBinLabel(37,"weight_tightbtagsf");
    counterhistSig->GetZaxis()->SetBinLabel(38,"weight_tightbtagsf_heavy_UP");
    counterhistSig->GetZaxis()->SetBinLabel(39,"weight_tightbtagsf_light_UP");
    counterhistSig->GetZaxis()->SetBinLabel(40,"weight_tightbtagsf_heavy_DN");
    counterhistSig->GetZaxis()->SetBinLabel(41,"weight_tightbtagsf_light_DN");
    counterhistSig->GetZaxis()->SetBinLabel(42,"weight_tightbtagsf_fastsim_UP");
    counterhistSig->GetZaxis()->SetBinLabel(43,"weight_tightbtagsf_fastsim_DN");
    counterhistSig->GetZaxis()->SetBinLabel(44,"weight_loosebtagsf");
    counterhistSig->GetZaxis()->SetBinLabel(45,"weight_loosebtagsf_heavy_UP");
    counterhistSig->GetZaxis()->SetBinLabel(46,"weight_loosebtagsf_light_UP");
    counterhistSig->GetZaxis()->SetBinLabel(47,"weight_loosebtagsf_heavy_DN");
    counterhistSig->GetZaxis()->SetBinLabel(48,"weight_loosebtagsf_light_DN");
    counterhistSig->GetZaxis()->SetBinLabel(49,"weight_loosebtagsf_fastsim_UP");
    counterhistSig->GetZaxis()->SetBinLabel(50,"weight_loosebtagsf_fastsim_DN");

    //histNEvts = new TH2F( "histNEvts", "h_histNEvts", 305,-1,1524, 205,-1,1024);//x=mStop, y=mLSP//65000 bins
    histNEvts = new TH2F( "histNEvts", "h_histNEvts", 81,-12.5,2012.5, 61,-12.5,1512.5);//x=mStop, y=mLSP//5000 bins
    histNEvts->Sumw2();
  }


  //
  //
  // Make Baby Ntuple
  //
  MakeBabyNtuple( Form("%s.root", output_name) );

  //
  // Initialize Baby Ntuple Branches
  //
  InitBabyNtuple();

  //
  // Set JSON file
  //
  const bool applyjson = false;
  if (applyjson) {
    const char* json_file = "json_files/Cert_294927-306462_13TeV_PromptReco_Collisions17_JSON.txt";
    set_goodrun_file_json(json_file);
  }

  //
  // Set scale1fb file
  //
  DatasetInfoFromFile df;
  df.loadFromFile("./scale1fbs.txt");  // load the default list shared across snt (soft-linked to CORE)
  df.loadFromFile("./scale1fbs_ext1.txt");  // load extra (private) samples and overwrite any conflict

  //
  // JEC files
  //
  std::vector<std::string> jetcorr_filenames_pfL1FastJetL2L3;
  FactorizedJetCorrector *jet_corrector_pfL1FastJetL2L3(0);
  JetCorrectionUncertainty* jetcorr_uncertainty(0);
  JetCorrectionUncertainty* jetcorr_uncertainty_sys(0);
  jetcorr_filenames_pfL1FastJetL2L3.clear();
  std::string jetcorr_uncertainty_filename;

  // JEC for AK8 Jets
  std::vector<std::string> ak8jetcorr_filenames_pfL1FastJetL2L3;
  FactorizedJetCorrector *ak8jet_corrector_pfL1FastJetL2L3(0);
  JetCorrectionUncertainty* ak8jetcorr_uncertainty(0);
  JetCorrectionUncertainty* ak8jetcorr_uncertainty_sys(0);
  ak8jetcorr_filenames_pfL1FastJetL2L3.clear();
  std::string ak8jetcorr_uncertainty_filename;

  // JEC file version
  string jecVer = gconf.jecEraMC;

  if (isDataFromFileName) {
    if      (filestr.find("2016B") != std::string::npos) jecVer = gconf.jecEraB;
    else if (filestr.find("2016C") != std::string::npos) jecVer = gconf.jecEraC;
    else if (filestr.find("2016D") != std::string::npos) jecVer = gconf.jecEraD;
    else if (filestr.find("2016E") != std::string::npos) jecVer = gconf.jecEraE;
    else if (filestr.find("2016F") != std::string::npos) jecVer = gconf.jecEraF;
    else if (filestr.find("2016G") != std::string::npos) jecVer = gconf.jecEraG;
    else if (filestr.find("2016H") != std::string::npos) jecVer = gconf.jecEraH;

    else if (filestr.find("2017B") != std::string::npos) jecVer = gconf.jecEraB;
    else if (filestr.find("2017C") != std::string::npos) jecVer = gconf.jecEraC;
    else if (filestr.find("2017D") != std::string::npos) jecVer = gconf.jecEraD;
    else if (filestr.find("2017E") != std::string::npos) jecVer = gconf.jecEraE;
    else if (filestr.find("2017F") != std::string::npos) jecVer = gconf.jecEraF;

    else if (filestr.find("2018A") != std::string::npos) jecVer = gconf.jecEraA;
    else if (filestr.find("2018B") != std::string::npos) jecVer = gconf.jecEraB;
    else if (filestr.find("2018C") != std::string::npos) jecVer = gconf.jecEraC;
    else if (filestr.find("2018D") != std::string::npos) jecVer = gconf.jecEraD;
    else {
      cout << "[babyMaker::looper] Cannot read from filestr the data era, may not be applying the correct JEC!\n";
    }
  }
  // 2016 Fastsim samples
  else if (isSignalFromFileName){
    jecVer = gconf.jecEraFS;
  }
  // ICHEP16 samples
  else if(filestr.find("Spring16") != std::string::npos){
    jecVer = "Spring16_25nsV6_MC";
  }

  jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L1FastJet_AK4PFchs.txt");
  jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L2Relative_AK4PFchs.txt");
  jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L3Absolute_AK4PFchs.txt");
  if (!isSignalFromFileName)
    jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L2L3Residual_AK4PFchs.txt");
  jetcorr_uncertainty_filename = "jecfiles/"+jecVer+"/"+jecVer+"_Uncertainty_AK4PFchs.txt";

  if (applyJECfromFile) {
    cout << "applying JEC from the following files:" << endl;
    for (auto filename : jetcorr_filenames_pfL1FastJetL2L3)
      cout << "   " << filename << endl;

    jet_corrector_pfL1FastJetL2L3 = makeJetCorrector(jetcorr_filenames_pfL1FastJetL2L3);
    jetcorr_uncertainty_sys = new JetCorrectionUncertainty(jetcorr_uncertainty_filename);
  } else {
    cout << "JECs taken from miniAOD directly" << endl;
  }

  // Prepare for future possible de-coupling applyJECunc from applyJECfromFile
  if (!isDataFromFileName && applyJECunc) {
    cout << "applying JEC uncertainties with weight " << applyJECunc << " from file: " << endl
         << "   " << jetcorr_uncertainty_filename << endl
         << "   " << ak8jetcorr_uncertainty_filename << endl;
    jetcorr_uncertainty = new JetCorrectionUncertainty(jetcorr_uncertainty_filename);
    ak8jetcorr_uncertainty = new JetCorrectionUncertainty(ak8jetcorr_uncertainty_filename);
  }

  if (applyAK8JECfromFile) {
    ak8jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L1FastJet_AK8PFPuppi.txt");
    ak8jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L2Relative_AK8PFPuppi.txt");
    ak8jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L3Absolute_AK8PFPuppi.txt");
    ak8jetcorr_filenames_pfL1FastJetL2L3.push_back("jecfiles/"+jecVer+"/"+jecVer+"_L2L3Residual_AK8PFPuppi.txt");
    ak8jetcorr_uncertainty_filename = "jecfiles/"+jecVer+"/"+jecVer+"_Uncertainty_AK8PFPuppi.txt";

    cout << "applying JEC from the following files:" << endl;
    for (auto filename : ak8jetcorr_filenames_pfL1FastJetL2L3)
      cout << "   " << filename << endl;

    ak8jet_corrector_pfL1FastJetL2L3 = makeJetCorrector(ak8jetcorr_filenames_pfL1FastJetL2L3);
    ak8jetcorr_uncertainty_sys = new JetCorrectionUncertainty(ak8jetcorr_uncertainty_filename);
  }

  //
  // Get bad events from txt files
  //
  // StopEvt.SetMetFilterEvents();
  // btagging scale factor//
  //
  jets.InitBtagSFTool(isFastsim);
  jets_jup.InitBtagSFTool(isFastsim);
  jets_jdown.InitBtagSFTool(isFastsim);
  // jets_jup.InitBtagSFTool(h_btag_eff_b,h_btag_eff_c,h_btag_eff_udsg, isFastsim);
  // jets_jdown.InitBtagSFTool(h_btag_eff_b,h_btag_eff_c,h_btag_eff_udsg, isFastsim);

  //
  // File Loop
  //
  while ( (currentFile = (TFile*)fileIter.Next()) ) {
    //
    // Get File Content
    //
    if(nEvents_processed >= nEventsToDo) continue;
    TFile* file = TFile::Open( currentFile->GetTitle() );
    TTree *tree = (TTree*)file->Get("Events");
    cms3.Init(tree);
    TString thisfilename = file->GetName();
    cout << "File name is " << file->GetName() << endl;

    const bool isbadrawMET = false;
    // if(thisfilename.Contains("V07-04-12_miniaodv1_FS")){
    //   cout << "This file seems to have a badrawMET, thus MET needs to be recalculated" << endl;
    //   isbadrawMET = true;
    // }

    //
    // Loop over Events in current file
    //
    unsigned int nEventsTree = tree->GetEntriesFast();

    for(unsigned int evt = 0; evt < nEventsTree; evt++){

      //
      // Get Event Content
      //
      if(nEvents_processed >= nEventsToDo) break;
      cms3.GetEntry(evt);
      nEvents_processed++;

      //
      // Progress
      //
      CMS3::progress(nEvents_processed, nEventsToDo);

      //
      // Intialize Baby NTuple Branches
      //
      InitBabyNtuple();

      counterhist->Fill(22,1.);

      //////////////////////////////////////////
      // If data, check against good run list//
      /////////////////////////////////////////
      if ( evt_isRealData() ) {
        if ( applyjson && !goodrun(evt_run(), evt_lumiBlock()) ) continue;
        DorkyEventIdentifier id(evt_run(), evt_event(), evt_lumiBlock());
        if ( is_duplicate(id) ) continue;
      }

      //
      // Fill Event Variables
      //
      //std::cout << "[babymaker::looper]: filling event vars" << std::endl;
      StopEvt.FillCommon(file->GetName());
      //std::cout << "[babymaker::looper]: filling event vars completed" << std::endl;
      //dumpDocLines();
      if(!StopEvt.is_data){
        StopEvt.weight_PU     = hPU    ->GetBinContent(hPU    ->FindBin(StopEvt.pu_ntrue));
        StopEvt.weight_PUup   = hPUup  ->GetBinContent(hPUup  ->FindBin(StopEvt.pu_ntrue));
        StopEvt.weight_PUdown = hPUdown->GetBinContent(hPUdown->FindBin(StopEvt.pu_ntrue));

        if (StopEvt.cms3tag.find("CMS4") == 0 && !isSignalFromFileName) {
          float sgnMCweight = (genps_weight() > 0)? 1 : -1;
          StopEvt.scale1fb = sgnMCweight * df.getScale1fbFromFile(StopEvt.dataset, StopEvt.cms3tag);
          StopEvt.xsec     = sgnMCweight * df.getXsecFromFile(StopEvt.dataset, StopEvt.cms3tag);
        }
      }
      float mStop = -1;
      float mLSP = -1;
      //This must come before any continue affecting signal scans
      if(isSignalFromFileName){
        //get stop and lsp mass from sparms
        if (gconf.cmssw_ver >= 94) {  // the sparm_names no longer exists after this
          if (sparm_values().size() < 2) cout << "[looper] ERROR: Sample is determined fastsim but not having 2 sparm values!!\n";
          StopEvt.mass_stop = sparm_values().at(0);
          StopEvt.mass_lsp  = sparm_values().at(1);
          if (isTChiFromFileName) StopEvt.mass_chargino = sparm_values().at(0);
        } else {
          for(unsigned int nsparm = 0; nsparm<sparm_names().size(); ++nsparm){
            //if(sparm_names().at(nsparm).Contains("mGluino")) StopEvt.mass_stop = sparm_values().at(nsparm);//dummy for testing as only T1's exist
            if(sparm_names().at(nsparm).Contains("mStop") ) StopEvt.mass_stop     = sparm_values().at(nsparm);
            if(sparm_names().at(nsparm).Contains("mCharg")) StopEvt.mass_chargino = sparm_values().at(nsparm);
            if(sparm_names().at(nsparm).Contains("mLSP")  ) StopEvt.mass_lsp      = sparm_values().at(nsparm);
            if(sparm_names().at(nsparm).Contains("mGl")   ) StopEvt.mass_gluino   = sparm_values().at(nsparm);
          }
        }
	//this is a stupid way of doing stuff, but it should be more stable
	mStop = StopEvt.mass_stop;
	if(     StopEvt.mass_stop<0&&StopEvt.mass_gluino>0)   mStop = StopEvt.mass_gluino;
	else if(StopEvt.mass_stop<0&&StopEvt.mass_chargino>0) mStop = StopEvt.mass_chargino;
	mLSP = StopEvt.mass_lsp;
        if (mStop < 0 || mLSP < 0) cout << "[looper] WARNING: Not getting valid signal mass points!! mStop = " << mStop << ", mLSP = " << mLSP << endl;
        if(genps_weight()>0)      histNEvts->Fill(mStop,mLSP,1);
        else if(genps_weight()<0) histNEvts->Fill(mStop,mLSP,-1);
        if(genps_weight()>0)      counterhistSig->Fill(mStop,mLSP,36,1);
        else if(genps_weight()<0) counterhistSig->Fill(mStop,mLSP,36,-1);
        StopEvt.xsec        = hxsec->GetBinContent(hxsec->FindBin(mStop));
        StopEvt.xsec_uncert = hxsec->GetBinError(  hxsec->FindBin(mStop));

        //Adjust for branching ratio built in to TChi W->lnu and H->bb samples
        if(isTChiFromFileName){
          StopEvt.xsec *= (0.324 * 0.584); //(pdg)
          StopEvt.xsec_uncert *= (0.324 * 0.584); //(pdg)
        }

        //note to get correct scale1fb you need to use in your looper xsec/nevt, where nevt you get via
        //histNEvts->GetBinContent(histNEvts->FindBin(mStop,mLSP));

        if (StopEvt.cms3tag.find("CMS4_V1") == 0) {
          // The correctly yearly normalized gen_LHEweight branches are available since CMS4_V10-02-04
          // note that the gen_LHEweight branches are already ratios wrt nominal
          // Q: will the LHE tool still be valid on the fastsim samples?
          StopEvt.pdf_up_weight      = gen_LHEweight_PDFVariation_Up();
          StopEvt.pdf_down_weight    = gen_LHEweight_PDFVariation_Dn();
          StopEvt.weight_Q2_up       = gen_LHEweight_QCDscale_muR2_muF2();
          StopEvt.weight_Q2_down     = gen_LHEweight_QCDscale_muR0p5_muF0p5();
          StopEvt.weight_muF_up      = gen_LHEweight_QCDscale_muR1_muF2();
          StopEvt.weight_muF_down    = gen_LHEweight_QCDscale_muR1_muF0p5();
          StopEvt.weight_muR_up      = gen_LHEweight_QCDscale_muR2_muF1();
          StopEvt.weight_muR_down    = gen_LHEweight_QCDscale_muR0p5_muF1();
          StopEvt.weight_alphas_up   = gen_LHEweight_AsMZ_Up();
          StopEvt.weight_alphas_down = gen_LHEweight_AsMZ_Dn();
          counterhist->Fill(1, genHEPMCweight()                    );
          counterhist->Fill(2, StopEvt.weight_muF_up               );
          counterhist->Fill(3, StopEvt.weight_muF_down             );
          counterhist->Fill(4, StopEvt.weight_muR_up               );
          counterhist->Fill(5, StopEvt.weight_Q2_up                );
          counterhist->Fill(6, gen_LHEweight_QCDscale_muR2_muF0p5());
          counterhist->Fill(7, StopEvt.weight_muR_down             );
          counterhist->Fill(8, gen_LHEweight_QCDscale_muR0p5_muF2());
          counterhist->Fill(9, StopEvt.weight_Q2_down              );
          counterhist->Fill(10,StopEvt.pdf_up_weight               );
          counterhist->Fill(11,StopEvt.pdf_down_weight             );
          counterhist->Fill(12,StopEvt.weight_alphas_up            );
          counterhist->Fill(13,StopEvt.weight_alphas_down          );
        } else {
          //copy from Mia's code
          float SMSpdf_weight_up = 1;
          float SMSpdf_weight_down = 1;
          float SMSsum_of_weights= 0;
          float SMSaverage_of_weights= 0;
          //error on pdf replicas
          //fastsim has first genweights bin being ==1
          StopEvt.ngenweights = genweights().size();
          if(genweights().size()>111){ //fix segfault
            for(int ipdf=10;ipdf<110;ipdf++){
              SMSaverage_of_weights += cms3.genweights().at(ipdf);
            }// average of weights
            SMSaverage_of_weights =  SMSaverage_of_weights/100.;
            for(int ipdf=10;ipdf<110;ipdf++){
              SMSsum_of_weights += pow(cms3.genweights().at(ipdf)- SMSaverage_of_weights,2);
            }//std of weights.
            SMSpdf_weight_up = (SMSaverage_of_weights+sqrt(SMSsum_of_weights/99.));
            SMSpdf_weight_down = (SMSaverage_of_weights-sqrt(SMSsum_of_weights/99.));

            double genw1inv = 1. / genweights()[1];  // inverse of the central gen weight 1
            StopEvt.pdf_up_weight      = SMSpdf_weight_up   * genw1inv;
            StopEvt.pdf_down_weight    = SMSpdf_weight_down * genw1inv;
            StopEvt.weight_Q2_up       = genweights()[5]    * genw1inv;
            StopEvt.weight_Q2_down     = genweights()[9]    * genw1inv;
            StopEvt.weight_alphas_up   = genweights()[110]  * genw1inv;
            StopEvt.weight_alphas_down = genweights()[111]  * genw1inv;

            counterhistSig->Fill(mStop,mLSP,1,genweights()[1]);
            counterhistSig->Fill(mStop,mLSP,2,genweights()[2]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,3,genweights()[3]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,4,genweights()[4]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,5,StopEvt.weight_Q2_up);
            counterhistSig->Fill(mStop,mLSP,6,genweights()[6]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,7,genweights()[7]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,8,genweights()[8]*genw1inv);
            counterhistSig->Fill(mStop,mLSP,9,StopEvt.weight_Q2_down);
            counterhistSig->Fill(mStop,mLSP,10,StopEvt.pdf_up_weight);
            counterhistSig->Fill(mStop,mLSP,11,StopEvt.pdf_down_weight);
            counterhistSig->Fill(mStop,mLSP,12,StopEvt.weight_alphas_up);
            counterhistSig->Fill(mStop,mLSP,13,StopEvt.weight_alphas_down);
          }
        }
      }// is signal
      else if(!evt_isRealData()){
        if (StopEvt.cms3tag.find("CMS4_V1") == 0) {
          // The correctly yearly normalized gen_LHEweight branches are available since CMS4_V10-02-04
          // note that the gen_LHEweight branches are already ratios wrt nominal
          StopEvt.pdf_up_weight      = gen_LHEweight_PDFVariation_Up();
          StopEvt.pdf_down_weight    = gen_LHEweight_PDFVariation_Dn();
          StopEvt.weight_Q2_up       = gen_LHEweight_QCDscale_muR2_muF2();
          StopEvt.weight_Q2_down     = gen_LHEweight_QCDscale_muR0p5_muF0p5();
          StopEvt.weight_muF_up      = gen_LHEweight_QCDscale_muR1_muF2();
          StopEvt.weight_muF_down    = gen_LHEweight_QCDscale_muR1_muF0p5();
          StopEvt.weight_muR_up      = gen_LHEweight_QCDscale_muR2_muF1();
          StopEvt.weight_muR_down    = gen_LHEweight_QCDscale_muR0p5_muF1();
          StopEvt.weight_alphas_up   = gen_LHEweight_AsMZ_Up();
          StopEvt.weight_alphas_down = gen_LHEweight_AsMZ_Dn();
          counterhist->Fill(1, genHEPMCweight()                    );
          counterhist->Fill(2, StopEvt.weight_muF_up               );
          counterhist->Fill(3, StopEvt.weight_muF_down             );
          counterhist->Fill(4, StopEvt.weight_muR_up               );
          counterhist->Fill(5, StopEvt.weight_Q2_up                );
          counterhist->Fill(6, gen_LHEweight_QCDscale_muR2_muF0p5());
          counterhist->Fill(7, StopEvt.weight_muR_down             );
          counterhist->Fill(8, gen_LHEweight_QCDscale_muR0p5_muF2());
          counterhist->Fill(9, StopEvt.weight_Q2_down              );
          counterhist->Fill(10,StopEvt.pdf_up_weight               );
          counterhist->Fill(11,StopEvt.pdf_down_weight             );
          counterhist->Fill(12,StopEvt.weight_alphas_up            );
          counterhist->Fill(13,StopEvt.weight_alphas_down          );
        } else {
          // calculate sum of weights and save them in a hisogram.
          float pdf_weight_up = 1;
          float pdf_weight_down = 1;
          float sum_of_weights= 0;
          float average_of_weights= 0;

          //error on pdf replicas
          // StopEvt.ngenweights = genweights().size();
          if(genweights().size()>110){
            for(int ipdf=9;ipdf<109;ipdf++){
              average_of_weights += cms3.genweights().at(ipdf);
            }// average of weights
            average_of_weights =  average_of_weights/100;

            for(int ipdf=9;ipdf<109;ipdf++){
              sum_of_weights += (cms3.genweights().at(ipdf)- average_of_weights)*(cms3.genweights().at(ipdf)-average_of_weights);
            }//std of weights.
            pdf_weight_up = (average_of_weights+sqrt(sum_of_weights/99));
            pdf_weight_down = (average_of_weights-sqrt(sum_of_weights/99));

            double genw0inv = 1. / genweights()[0];  // inverse of the central gen weight [0]
            StopEvt.pdf_up_weight      = pdf_weight_up     * genw0inv;
            StopEvt.pdf_down_weight    = pdf_weight_down   * genw0inv;
            StopEvt.weight_Q2_up       = genweights()[4]   * genw0inv;
            StopEvt.weight_Q2_down     = genweights()[8]   * genw0inv;
            StopEvt.weight_alphas_up   = genweights()[109] * genw0inv;
            StopEvt.weight_alphas_down = genweights()[110] * genw0inv;
            counterhist->Fill(1,genweights()[0]);
            counterhist->Fill(2,genweights()[1]*genw0inv);
            counterhist->Fill(3,genweights()[2]*genw0inv);
            counterhist->Fill(4,genweights()[3]*genw0inv);
            counterhist->Fill(5,StopEvt.weight_Q2_up);
            counterhist->Fill(6,genweights()[5]*genw0inv);
            counterhist->Fill(7,genweights()[6]*genw0inv);
            counterhist->Fill(8,genweights()[7]*genw0inv);
            counterhist->Fill(9,StopEvt.weight_Q2_down);
            counterhist->Fill(10,StopEvt.pdf_up_weight);
            counterhist->Fill(11,StopEvt.pdf_down_weight);
            counterhist->Fill(12,StopEvt.weight_alphas_up);
            counterhist->Fill(13,StopEvt.weight_alphas_down);
          }
        }
      }

      //
      // Gen Information - now goes first
      //
      //std::cout << "[babymaker::looper]: filling gen particles vars" << std::endl;

      //ttbar counters using neutrinos:
      int n_nutaufromt=0;
      int n_nuelfromt=0;
      int n_numufromt=0;

      int nLepsHardProcess=0;
      int nNusHardProcess=0;

      bool ee0lep=false;
      bool ee1lep=false;
      bool ge2lep=false;
      bool zToNuNu=false;

      TString thisFile = chain->GetFile()->GetName();
      bool isstopevent = false;
      bool istopevent = false;
      bool isWevent = false;
      bool isZevent = false;
      // vector<LorentzVector> genpnotISR; genpnotISR.clear();

      if(thisFile.Contains("DYJets") || thisFile.Contains("ZJets") ||
         thisFile.Contains("ZZ") || thisFile.Contains("WZ") ||
         thisFile.Contains("TTZ") || thisFile.Contains("tZq") )
        isZevent = true;
      if(thisFile.Contains("WJets") ||
         thisFile.Contains("WW") || thisFile.Contains("WZ") ||
         thisFile.Contains("TTW") )
        isWevent = true;
      if(thisFile.Contains("TTJets") || thisFile.Contains("TTTo2L2Nu") || thisFile.Contains("TT_") ||
         thisFile.Contains("ST_") || thisFile.Contains("tZq") ||
         thisFile.Contains("TTW") || thisFile.Contains("TTZ") )
        istopevent = true;
      if(isSignalFromFileName ||
         thisFile.Contains("T2tt") || thisFile.Contains("T2tb") || thisFile.Contains("T2bW") )
        isstopevent = true;

      //gen particles
      if (!evt_isRealData()){
        for(unsigned int genx = 0; genx < genps_p4().size() ; genx++){
          if( genps_id().at(genx)==genps_id_simplemother().at(genx) && !genps_isLastCopy().at(genx) ) continue;

          if( genps_isHardProcess().at(genx) || genps_fromHardProcessDecayed().at(genx) || genps_fromHardProcessFinalState().at(genx) ||
              genps_isLastCopy().at(genx) || genps_status().at(genx)==1 ) {

            int pdgid = abs(genps_id().at(genx));
            int mother_id = abs(genps_id_mother().at(genx));
            int mother_idx =  genps_idx_mother().at(genx);

            // if( pdgid == pdg_el || pdgid == pdg_mu || pdgid == pdg_tau || pdgid == pdg_b || pdgid == pdg_c || pdgid == pdg_s || pdgid == pdg_d || pdgid == pdg_u ) {
            //   // if have lepton or quark --> those could be matched to a jet
            //   if(mother_id==pdg_t || mother_id==pdg_W || mother_id==1000006 || mother_id==2000006 || mother_id==pdg_chi_1plus1 || mother_id==pdg_chi_2plus1){
            //     genpnotISR.push_back(genps_p4().at(genx));
            //   }
            // }

            if( pdgid == pdg_el  ||  pdgid == pdg_mu || pdgid == pdg_tau) gen_leps.FillCommon(genx);
            if( pdgid == pdg_numu || pdgid == pdg_nue || pdgid == pdg_nutau ) gen_nus.FillCommon(genx);
            if( pdgid == pdg_t || pdgid == pdg_b || pdgid == pdg_c || pdgid == pdg_s || pdgid == pdg_d ||pdgid == pdg_u   ) gen_qs.FillCommon(genx);
            if( pdgid == pdg_W || pdgid == pdg_Z || (pdgid == pdg_ph && genps_p4().at(genx).Pt()>5.0)  || pdgid == pdg_h  ) gen_bosons.FillCommon(genx);

            //add all SUSY particles
            if(pdgid >= 1000000&&pdgid <= 1000040) gen_susy.FillCommon(genx);

            if( mother_id == pdg_W && pdgid == pdg_nue && genps_status().at(genx) == 1 && abs(genps_id_mother().at(mother_idx)) == pdg_t) n_nuelfromt++;
            if( mother_id == pdg_W && pdgid == pdg_numu && genps_status().at(genx) == 1 && abs(genps_id_mother().at(mother_idx)) == pdg_t ) n_numufromt++;
            if( mother_id == pdg_W && pdgid == pdg_nutau && genps_status().at(genx) == 1 && abs(genps_id_mother().at(mother_idx)) == pdg_t ) n_nutaufromt++;

            if( pdgid == pdg_el  && genps_fromHardProcessFinalState().at(genx) && genps_isLastCopy().at(genx) ) nLepsHardProcess++;
            if( pdgid == pdg_mu  && genps_fromHardProcessFinalState().at(genx) && genps_isLastCopy().at(genx) ) nLepsHardProcess++;
            if( pdgid == pdg_tau && genps_fromHardProcessDecayed().at(genx) && genps_isLastCopy().at(genx) ) nLepsHardProcess++;

            if( pdgid == pdg_nue   && genps_fromHardProcessFinalState().at(genx) && genps_isLastCopy().at(genx) ) nNusHardProcess++;
            if( pdgid == pdg_numu  && genps_fromHardProcessFinalState().at(genx) && genps_isLastCopy().at(genx) ) nNusHardProcess++;
            if( pdgid == pdg_nutau && genps_fromHardProcessFinalState().at(genx) && genps_isLastCopy().at(genx) ) nNusHardProcess++;

          }
        }
        //use babies genparticles
        LorentzVector hardsystem;
        hardsystem.SetPxPyPzE(0.,0.,0.,0.);
        if(isstopevent){
          for(unsigned int genx = 0; genx < gen_susy.id.size() ; genx++){
            if(abs(gen_susy.id.at(genx))==pdg_stop1 && gen_susy.isLastCopy.at(genx)) hardsystem += gen_susy.p4.at(genx);
          }
        }
        if(istopevent){
          for(unsigned int genx = 0; genx < gen_qs.id.size() ; genx++){
            if(abs(gen_qs.id.at(genx))==pdg_t && gen_qs.isLastCopy.at(genx)) hardsystem += gen_qs.p4.at(genx);
          }
        }
        if(isWevent){
          for(unsigned int genx = 0; genx < gen_bosons.id.size() ; genx++){
            if(abs(gen_bosons.id.at(genx))==pdg_W && abs(gen_bosons.id.at(genx))!=pdg_t && gen_bosons.isLastCopy.at(genx)) hardsystem +=+ gen_bosons.p4.at(genx);
          }
        }
        if(isZevent){
          for(unsigned int genx = 0; genx < gen_bosons.id.size() ; genx++){
            if(abs(gen_bosons.id.at(genx))==pdg_Z && abs(gen_bosons.id.at(genx))!=pdg_t && gen_bosons.isLastCopy.at(genx)) hardsystem += gen_bosons.p4.at(genx);
          }
        }
        StopEvt.hardgenpt = hardsystem.Pt();
        StopEvt.weight_ISR = 1.;
        //caution - hardcoded
        //note - these weights don't contain the renormalization
        if(StopEvt.hardgenpt>600.) {
          StopEvt.weight_ISRup = 1.3;
          StopEvt.weight_ISRdown = 0.7;
        } else if(StopEvt.hardgenpt>400.) {
          StopEvt.weight_ISRup = 1.15;
          StopEvt.weight_ISRdown = 0.85;
        } else {
          StopEvt.weight_ISRup = 1.;
          StopEvt.weight_ISRdown = 1.;
        }
        counterhist->Fill(19,StopEvt.weight_ISR);
        counterhist->Fill(20,StopEvt.weight_ISRup);
        counterhist->Fill(21,StopEvt.weight_ISRdown);
        if(isSignalFromFileName){
          counterhistSig->Fill(mStop,mLSP,19,StopEvt.weight_ISR);
          counterhistSig->Fill(mStop,mLSP,20,StopEvt.weight_ISRup);
          counterhistSig->Fill(mStop,mLSP,21,StopEvt.weight_ISRdown);
        }
      }//no data

      // Gen lepton counting and event classification
      StopEvt.genlepsfromtop = n_nuelfromt+n_numufromt+n_nutaufromt;
      gen_leps.gen_nfromt = n_nuelfromt+ n_numufromt + n_nutaufromt;
      //gen_els.gen_nfromt = n_nuelfromt;
      //gen_mus.gen_nfromt = n_numufromt;
      //gen_taus.gen_nfromt = n_nutaufromt;

      StopEvt.genLepsHardProcess = nLepsHardProcess;
      StopEvt.genNusHardProcess  = nNusHardProcess;

      if(nLepsHardProcess==0) ee0lep=true;
      if(nLepsHardProcess==1) ee1lep=true;
      if(nLepsHardProcess>=2) ge2lep=true;

      if( thisFile.Contains("DYJets") ||
          thisFile.Contains("ZJets")  ||
          thisFile.Contains("ZZ")        ){
        if(nNusHardProcess>=2) zToNuNu=true;
      }
      if( thisFile.Contains("WZ")  ||
          thisFile.Contains("TTZ") ||
          thisFile.Contains("tZq")    ){
        if(nNusHardProcess-nLepsHardProcess>=2) zToNuNu=true;
      }

      if( zToNuNu )    { StopEvt.isZtoNuNu=1; StopEvt.is0lep=0; StopEvt.is1lep=0; StopEvt.is2lep=0; }
      else if( ee0lep ){ StopEvt.isZtoNuNu=0; StopEvt.is0lep=1; StopEvt.is1lep=0; StopEvt.is2lep=0; }
      else if( ee1lep ){ StopEvt.isZtoNuNu=0; StopEvt.is0lep=0; StopEvt.is1lep=1; StopEvt.is2lep=0; }
      else if( ge2lep ){ StopEvt.isZtoNuNu=0; StopEvt.is0lep=0; StopEvt.is1lep=0; StopEvt.is2lep=1; }

      if( (ee1lep) && ((StopEvt.genLepsHardProcess-StopEvt.genlepsfromtop)>0) )  StopEvt.is1lepFromW=1;
      else StopEvt.is1lepFromW=0;

      if( (ee1lep) && ((StopEvt.genLepsHardProcess-StopEvt.genlepsfromtop)==0) ) StopEvt.is1lepFromTop=1;
      else StopEvt.is1lepFromTop=0;

      //
      // nVertex Cut
      //
      if(StopEvt.nvtxs < skim_nvtx) continue;
      if(StopEvt.firstGoodVtxIdx!=0) continue; //really check that first vertex is good
      nEvents_pass_skim_nVtx++;
      //keep those here - any event without vertex is BS
      //save met here because of JEC
      if(applyJECfromFile){
        int useCleanedMET = 0;
        if (applyMETRecipeV2 && gconf.year == 2017) {
          useCleanedMET = 2;
          if (!thisFile.Contains("09May2018")) {
            StopEvt.pfmet_original = evt_old_pfmet_raw();
            StopEvt.pfmet_original_phi = evt_old_pfmetPhi_raw();
          }
        }
        pair<float,float> newmet;
        pair<float,float> newmet_jup;
        pair<float,float> newmet_jdown;
        if(!evt_isRealData() && applyJECunc){
          if      (JES_type > 0) newmet = getT1CHSMET_fromMINIAOD(jet_corrector_pfL1FastJetL2L3, jetcorr_uncertainty, true, isbadrawMET, useCleanedMET);
          else if (JES_type < 0) newmet = getT1CHSMET_fromMINIAOD(jet_corrector_pfL1FastJetL2L3, jetcorr_uncertainty, false, isbadrawMET, useCleanedMET);
          else cout << "This should not happen" << endl;
        }
        else newmet = getT1CHSMET_fromMINIAOD(jet_corrector_pfL1FastJetL2L3, nullptr, 0, isbadrawMET, useCleanedMET);

        newmet_jup   = getT1CHSMET_fromMINIAOD(jet_corrector_pfL1FastJetL2L3, jetcorr_uncertainty_sys, true, isbadrawMET, useCleanedMET);
        newmet_jdown = getT1CHSMET_fromMINIAOD(jet_corrector_pfL1FastJetL2L3, jetcorr_uncertainty_sys, false, isbadrawMET, useCleanedMET);

        StopEvt.pfmet = newmet.first;
        StopEvt.pfmet_phi = newmet.second;
        if(evt_isRealData() && thisFile.Contains("03Feb2017")){
          StopEvt.pfmet = evt_muegclean_pfmet();
          StopEvt.pfmet_phi = evt_muegclean_pfmetPhi();
          StopEvt.pfmet_original = newmet.first;
          StopEvt.pfmet_original_phi = newmet.second;
        }
        StopEvt.pfmet_jup = newmet_jup.first;
        StopEvt.pfmet_phi_jup = newmet_jup.second;
        StopEvt.pfmet_jdown = newmet_jdown.first;
        StopEvt.pfmet_phi_jdown = newmet_jdown.second;
        if(TMath::IsNaN(StopEvt.pfmet)||(!TMath::Finite(StopEvt.pfmet))||StopEvt.pfmet>14000.) {
          cout << "Invalid MET value after correction, skipping event! run:lumi:event = " << evt_run() << ":" << evt_lumiBlock() << ":" << evt_event()
               << ", evt_pfmet()= " << evt_pfmet() << ", StopEvt.pfmet= " << StopEvt.pfmet << endl;
          continue;
        }
      }
      else{
        StopEvt.pfmet = evt_pfmet();
        StopEvt.pfmet_phi = evt_pfmetPhi();
        StopEvt.pfmet_jup = evt_pfmet_JetResUp();
        StopEvt.pfmet_phi_jup = evt_pfmetPhi_JetResUp();
        StopEvt.pfmet_jdown = evt_pfmet_JetResDown();
        StopEvt.pfmet_phi_jdown = evt_pfmetPhi_JetResDown();
        if (gconf.year == 2017 && !thisFile.Contains("09May2018")) {
          StopEvt.pfmet_original = evt_old_pfmet_raw();
          StopEvt.pfmet_original_phi = evt_old_pfmetPhi_raw();
        }
        if(evt_isRealData() && thisFile.Contains("03Feb2017")){
          StopEvt.pfmet = evt_muegclean_pfmet();
          StopEvt.pfmet_phi = evt_muegclean_pfmetPhi();
          StopEvt.pfmet_original = evt_pfmet();
          StopEvt.pfmet_original_phi = evt_pfmetPhi();
        }
      }
      if(evt_isRealData() && thisFile.Contains("03Feb2017")){
        StopEvt.pfmet_egclean = evt_egclean_pfmet();
        StopEvt.pfmet_egclean_phi = evt_egclean_pfmetPhi();
        StopEvt.pfmet_muegclean = evt_muegclean_pfmet();
        StopEvt.pfmet_muegclean_phi = evt_muegclean_pfmetPhi();
        StopEvt.pfmet_muegcleanfix = evt_muegcleanfix_pfmet();
        StopEvt.pfmet_muegcleanfix_phi = evt_muegcleanfix_pfmetPhi();
        StopEvt.pfmet_uncorr = evt_uncorr_pfmet();
        StopEvt.pfmet_uncorr_phi = evt_uncorr_pfmetPhi();
      }

      StopEvt.filt_pfovercalomet = !(StopEvt.calomet>0&&StopEvt.pfmet/StopEvt.calomet>5);

      //
      //Lepton Variables
      //
      //std::cout << "[babymaker::looper]: selecting leptons" << std::endl;
      int nGoodLeptons = 0;  //stored lep1,lep2
      int nVetoLeptons = 0;

      vector<Lepton> GoodLeps;
      vector<Lepton> LooseLeps;
      vector<Lepton> VetoLeps;
      vector<Lepton> AllLeps;
      vector<unsigned int> idx_alloverlapjets;
      vector<unsigned int> idx_alloverlapjets_jup;
      vector<unsigned int> idx_alloverlapjets_jdown;

      // Electrons
      for (unsigned int eidx = 0; eidx < els_p4().size(); eidx++){

        if( !PassElectronVetoSelections(eidx, skim_vetoLep_el_pt, skim_vetoLep_el_eta) ) continue;

        Lepton mylepton;
        mylepton.id  = -11*els_charge().at(eidx);
        mylepton.idx = eidx;
        mylepton.p4  = els_p4().at(eidx);
        int overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,applyJECfromFile,jetcorr_uncertainty,JES_type,isFastsim);  //don't care about jid
        if( overlapping_jet>=0) idx_alloverlapjets.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
        if(applyJECfromFile) {//if no JEC file is defined - code cannot do up/down variations
          overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,1,isFastsim);  //don't care about jid
          if( overlapping_jet>=0) idx_alloverlapjets_jup.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
          overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,-1,isFastsim);  //don't care about jid
          if( overlapping_jet>=0) idx_alloverlapjets_jdown.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
        }

        if( ( PassElectronVetoSelections(eidx, skim_vetoLep_el_pt, skim_vetoLep_el_eta) ) &&
            (!PassElectronPreSelections(eidx, skim_looseLep_el_pt, skim_looseLep_el_eta) )    ) VetoLeps.push_back(mylepton);

        if( ( PassElectronPreSelections(eidx, skim_looseLep_el_pt, skim_looseLep_el_eta) ) &&
            (!PassElectronPreSelections(eidx, skim_goodLep_el_pt, skim_goodLep_el_eta) )      ) LooseLeps.push_back(mylepton);

        if( PassElectronPreSelections(eidx, skim_goodLep_el_pt, skim_goodLep_el_eta)          ) GoodLeps.push_back(mylepton);

      }

      // Muons
      for (unsigned int midx = 0; midx < mus_p4().size(); midx++){

        if( !PassMuonVetoSelections(midx, skim_vetoLep_mu_pt, skim_vetoLep_mu_eta) ) continue;

        Lepton mylepton;
        mylepton.id  = -13*mus_charge().at(midx);
        mylepton.idx = midx;
        mylepton.p4  = mus_p4().at(midx);
        int overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4() , 0.4, skim_jet_pt, skim_jet_eta,false,jet_corrector_pfL1FastJetL2L3,applyJECfromFile,jetcorr_uncertainty,JES_type,isFastsim);  //don't care about jid
        if( overlapping_jet>=0) idx_alloverlapjets.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
        if(applyJECfromFile) {//if no JEC file is defined - code cannot do up/down variations
          overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4() , 0.4, skim_jet_pt, skim_jet_eta,false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,1,isFastsim);  //don't care about jid
          if( overlapping_jet>=0) idx_alloverlapjets_jup.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
          overlapping_jet = getOverlappingJetIndex(mylepton.p4, pfjets_p4() , 0.4, skim_jet_pt, skim_jet_eta,false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,-1,isFastsim);  //don't care about jid
          if( overlapping_jet>=0) idx_alloverlapjets_jdown.push_back(overlapping_jet);  //overlap removal for all jets w.r.t. all leptons
        }

        if( ( PassMuonVetoSelections(midx, skim_vetoLep_mu_pt, skim_vetoLep_mu_eta) ) &&
            (!PassMuonPreSelections(midx, skim_looseLep_mu_pt, skim_looseLep_mu_eta) )    ) VetoLeps.push_back(mylepton);

        if( ( PassMuonPreSelections(midx, skim_looseLep_mu_pt, skim_looseLep_mu_eta) ) &&
            (!PassMuonPreSelections(midx, skim_goodLep_mu_pt, skim_goodLep_mu_eta) )      ) LooseLeps.push_back(mylepton);

        if( PassMuonPreSelections(midx, skim_goodLep_mu_pt, skim_goodLep_mu_eta)          ) GoodLeps.push_back(mylepton);

      }
      sort(GoodLeps.begin(),GoodLeps.end(),sortLepbypt());
      sort(LooseLeps.begin(),LooseLeps.end(),sortLepbypt());
      sort(VetoLeps.begin(),VetoLeps.end(),sortLepbypt());

      if(GoodLeps.size()>0){
        for(unsigned int lep = 1; lep<GoodLeps.size(); lep++){
          if(ROOT::Math::VectorUtil::DeltaR(GoodLeps.at(0).p4,GoodLeps.at(lep).p4)<0.01) GoodLeps.erase(GoodLeps.begin()+lep);
        }
        for(unsigned int lep = 0; lep<LooseLeps.size(); lep++){
          if(ROOT::Math::VectorUtil::DeltaR(GoodLeps.at(0).p4,LooseLeps.at(lep).p4)<0.01) LooseLeps.erase(LooseLeps.begin()+lep);
        }
        for(unsigned int lep = 0; lep<VetoLeps.size(); lep++){
          if(ROOT::Math::VectorUtil::DeltaR(GoodLeps.at(0).p4,VetoLeps.at(lep).p4)<0.01) VetoLeps.erase(VetoLeps.begin()+lep);
        }
      }
      nGoodLeptons = GoodLeps.size();
      int nLooseLeptons = GoodLeps.size() + LooseLeps.size();//use for Zll
      nVetoLeptons = GoodLeps.size() + LooseLeps.size() + VetoLeps.size();

      StopEvt.ngoodleps  = nGoodLeptons;
      // StopEvt.nlooseleps = nLooseLeptons; //why are these needed?
      StopEvt.nvetoleps  = nVetoLeptons;

      //std::cout << "[babymaker::looper]: filling lepton variables" << std::endl;
      AllLeps.clear();
      AllLeps.insert( AllLeps.end(), GoodLeps.begin(),  GoodLeps.end() );
      AllLeps.insert( AllLeps.end(), LooseLeps.begin(), LooseLeps.end() );
      AllLeps.insert( AllLeps.end(), VetoLeps.begin(),  VetoLeps.end() );
      if( nVetoLeptons > 0 ) lep1.FillCommon( AllLeps.at(0).id, AllLeps.at(0).idx );
      if( nVetoLeptons > 1 ) lep2.FillCommon( AllLeps.at(1).id, AllLeps.at(1).idx );

      //fill lep-dphi
      if( nVetoLeptons > 0 ) {
        lep1.dphiMET = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi);
        if(applyJECfromFile){
          lep1.dphiMET_jup = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi_jup);
          lep1.dphiMET_jdown = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi_jdown);
        }
      }
      if( nVetoLeptons > 1 ) {
        lep2.dphiMET = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi);
        if(applyJECfromFile){
          lep2.dphiMET_jup = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi_jup);
          lep2.dphiMET_jdown = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi_jdown);
        }
      }

      // Lepton SFs
      double lepSF    = 1.0;
      double lepSF_Up = 1.0;
      double lepSF_Dn = 1.0;

      double vetoLepSF    = 1.0;
      double vetoLepSF_Up = 1.0;
      double vetoLepSF_Dn = 1.0;

      double lepSF_FS    = 1.0;
      double lepSF_FS_Up = 1.0;
      double lepSF_FS_Dn = 1.0;

      if (applyLeptonSFs && !StopEvt.is_data && nVetoLeptons > 0) {
        std::vector<float> recoLep_pt, recoLep_eta, genLostLep_pt, genLostLep_eta;
        std::vector<int> recoLep_pdgid, genLostLep_pdgid;
        std::vector<bool> recoLep_isSel;

        if ( nVetoLeptons > 0 ) {
          recoLep_pt.push_back( lep1.p4.Pt() );
          recoLep_eta.push_back( (abs(lep1.pdgid) == 11)? lep1.etaSC : lep1.p4.Eta() );
          recoLep_pdgid.push_back( lep1.pdgid );
          recoLep_isSel.push_back( nGoodLeptons > 0 );
          if ( nVetoLeptons > 1 ) {
            recoLep_pt.push_back( lep2.p4.Pt() );
            recoLep_eta.push_back( (abs(lep2.pdgid) == 11)? lep2.etaSC : lep2.p4.Eta() );
            recoLep_pdgid.push_back( lep2.pdgid );
            recoLep_isSel.push_back( nGoodLeptons > 1 );
          } // end if >=2 vetoLeptons
        } // end if >=1 vetoLeptons

        const double matched_dr = 0.1;

        // Find Gen Lost lepton
        if ( StopEvt.is2lep && nVetoLeptons == 1 ) {
          // Find gen lepton matched to reco lepton
          for (int iGen=0; iGen<(int)gen_leps.p4.size(); iGen++) {
            if ( abs(gen_leps.id.at(iGen))!=11 && abs(gen_leps.id.at(iGen))!=13 ) continue;
            if ( !gen_leps.fromHardProcessFinalState.at(iGen) ) continue;
            if ( !gen_leps.isLastCopy.at(iGen) ) continue;
            if ( ROOT::Math::VectorUtil::DeltaR(gen_leps.p4.at(iGen), lep1.p4)<matched_dr ) continue;
            if ( gen_leps.p4.at(iGen).Pt()<5 || fabs(gen_leps.p4.at(iGen).Eta())>2.4 ) continue;

            genLostLep_pt.push_back( gen_leps.p4.at(iGen).Pt() );
            genLostLep_eta.push_back( gen_leps.p4.at(iGen).Eta() );
            genLostLep_pdgid.push_back( gen_leps.id.at(iGen) );
          } // end loop over genleps
        } // end if lostLep event

        lepsf.getLepWeight(recoLep_pt, recoLep_eta, recoLep_pdgid, recoLep_isSel, genLostLep_pt, genLostLep_eta, genLostLep_pdgid,
                           lepSF, lepSF_Up, lepSF_Dn, lepSF_FS, lepSF_FS_Up, lepSF_FS_Dn, vetoLepSF, vetoLepSF_Up, vetoLepSF_Dn);
      }

      StopEvt.weight_lepSF      = lepSF;
      StopEvt.weight_lepSF_up   = lepSF_Up;
      StopEvt.weight_lepSF_down = lepSF_Dn;
      if(lepSF==0) cout << "[looper]>> LepSF is 0!" << endl;

      StopEvt.weight_vetoLepSF      = vetoLepSF;
      StopEvt.weight_vetoLepSF_up   = vetoLepSF_Up;
      StopEvt.weight_vetoLepSF_down = vetoLepSF_Dn;

      StopEvt.weight_lepSF_fastSim      = lepSF_FS;
      StopEvt.weight_lepSF_fastSim_up   = lepSF_FS_Up;
      StopEvt.weight_lepSF_fastSim_down = lepSF_FS_Dn;

      // save the sum of weights for normalization offline to n-babies.
      // comment: this has to go before the selection cuts - else you create a bias
      if(!evt_isRealData() && applyLeptonSFs) {
        counterhist->Fill(28,StopEvt.weight_lepSF);
        counterhist->Fill(29,StopEvt.weight_lepSF_up);
        counterhist->Fill(30,StopEvt.weight_lepSF_down);
      }
      if(!evt_isRealData() && applyVetoLeptonSFs){
        counterhist->Fill(31,StopEvt.weight_vetoLepSF);
        counterhist->Fill(32,StopEvt.weight_vetoLepSF_up);
        counterhist->Fill(33,StopEvt.weight_vetoLepSF_down);
      }
      if(!evt_isRealData() && applyLeptonSFs && isFastsim){
        counterhist->Fill(34,StopEvt.weight_lepSF_fastSim);
        counterhist->Fill(35,StopEvt.weight_lepSF_fastSim_up);
        counterhist->Fill(36,StopEvt.weight_lepSF_fastSim_down);
      }

      // Signal
      if(isSignalFromFileName && !evt_isRealData() && applyLeptonSFs) {
        counterhistSig->Fill(mStop,mLSP,27,StopEvt.weight_lepSF);
        counterhistSig->Fill(mStop,mLSP,28,StopEvt.weight_lepSF_up);
        counterhistSig->Fill(mStop,mLSP,29,StopEvt.weight_lepSF_down);
      }
      if(isSignalFromFileName && !evt_isRealData() && applyVetoLeptonSFs){
        counterhistSig->Fill(mStop,mLSP,30,StopEvt.weight_vetoLepSF);
        counterhistSig->Fill(mStop,mLSP,31,StopEvt.weight_vetoLepSF_up);
        counterhistSig->Fill(mStop,mLSP,32,StopEvt.weight_vetoLepSF_down);
      }
      if(isSignalFromFileName && !evt_isRealData() && applyLeptonSFs && isFastsim){
        counterhistSig->Fill(mStop,mLSP,33,StopEvt.weight_lepSF_fastSim);
        counterhistSig->Fill(mStop,mLSP,34,StopEvt.weight_lepSF_fastSim_up);
        counterhistSig->Fill(mStop,mLSP,35,StopEvt.weight_lepSF_fastSim_down);
      }


      //
      // Jet Selection
      float btagprob_data = 1.;
      float btagprob_mc = 1.;
      float btagprob_heavy_UP = 1.;
      float btagprob_heavy_DN = 1.;
      float btagprob_light_UP = 1.;
      float btagprob_light_DN = 1.;
      float btagprob_FS_UP = 1.;
      float btagprob_FS_DN = 1.;

      float btagprob_data_jup = 1.;
      float btagprob_mc_jup  = 1.;
      float btagprob_heavy_UP_jup  = 1.;
      float btagprob_heavy_DN_jup  = 1.;
      float btagprob_light_UP_jup  = 1.;
      float btagprob_light_DN_jup  = 1.;
      float btagprob_FS_UP_jup  = 1.;
      float btagprob_FS_DN_jup  = 1.;
      float btagprob_data_jdown = 1.;
      float btagprob_mc_jdown = 1.;
      float btagprob_heavy_UP_jdown = 1.;
      float btagprob_heavy_DN_jdown = 1.;
      float btagprob_light_UP_jdown = 1.;
      float btagprob_light_DN_jdown = 1.;
      float btagprob_FS_UP_jdown = 1.;
      float btagprob_FS_DN_jdown = 1.;

      float loosebtagprob_data = 1.;
      float loosebtagprob_mc = 1.;
      float loosebtagprob_heavy_UP = 1.;
      float loosebtagprob_heavy_DN = 1.;
      float loosebtagprob_light_UP = 1.;
      float loosebtagprob_light_DN = 1.;
      float loosebtagprob_FS_UP = 1.;
      float loosebtagprob_FS_DN = 1.;
      float tightbtagprob_data = 1.;
      float tightbtagprob_mc = 1.;
      float tightbtagprob_heavy_UP = 1.;
      float tightbtagprob_heavy_DN = 1.;
      float tightbtagprob_light_UP = 1.;
      float tightbtagprob_light_DN = 1.;
      float tightbtagprob_FS_UP = 1.;
      float tightbtagprob_FS_DN = 1.;

      float loosebtagprob_data_jup = 1.;
      float loosebtagprob_mc_jup = 1.;
      float loosebtagprob_heavy_UP_jup = 1.;
      float loosebtagprob_heavy_DN_jup = 1.;
      float loosebtagprob_light_UP_jup = 1.;
      float loosebtagprob_light_DN_jup = 1.;
      float loosebtagprob_FS_UP_jup = 1.;
      float loosebtagprob_FS_DN_jup = 1.;
      float tightbtagprob_data_jup = 1.;
      float tightbtagprob_mc_jup = 1.;
      float tightbtagprob_heavy_UP_jup = 1.;
      float tightbtagprob_heavy_DN_jup = 1.;
      float tightbtagprob_light_UP_jup = 1.;
      float tightbtagprob_light_DN_jup = 1.;
      float tightbtagprob_FS_UP_jup = 1.;
      float tightbtagprob_FS_DN_jup = 1.;

      float loosebtagprob_data_jdown = 1.;
      float loosebtagprob_mc_jdown = 1.;
      float loosebtagprob_heavy_UP_jdown = 1.;
      float loosebtagprob_heavy_DN_jdown = 1.;
      float loosebtagprob_light_UP_jdown = 1.;
      float loosebtagprob_light_DN_jdown = 1.;
      float loosebtagprob_FS_UP_jdown = 1.;
      float loosebtagprob_FS_DN_jdown = 1.;
      float tightbtagprob_data_jdown = 1.;
      float tightbtagprob_mc_jdown = 1.;
      float tightbtagprob_heavy_UP_jdown = 1.;
      float tightbtagprob_heavy_DN_jdown = 1.;
      float tightbtagprob_light_UP_jdown = 1.;
      float tightbtagprob_light_DN_jdown = 1.;
      float tightbtagprob_FS_UP_jdown = 1.;
      float tightbtagprob_FS_DN_jdown = 1.;

      //std::cout << "[babymaker::looper]: filling jets vars" << std::endl;
      // Get the jets overlapping with the selected leptons
      if(pfjets_p4().size() > 0){
        jet_overlep1_idx = -9999;
        jet_overlep2_idx = -9999;
        //applyJECfromFile=false;
        if(nVetoLeptons>0) jet_overlep1_idx = getOverlappingJetIndex(lep1.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,applyJECfromFile,jetcorr_uncertainty,JES_type,isFastsim);  //don't care about jid
        if(nVetoLeptons>1) jet_overlep2_idx = getOverlappingJetIndex(lep2.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,applyJECfromFile,jetcorr_uncertainty,JES_type,isFastsim);  //don't care about jid

        // Jets and b-tag variables feeding the index for the jet overlapping the selected leptons
        jets.SetJetSelection("ak4", skim_jet_pt, skim_jet_eta, true, analysis_jet_pt, analysis_bjet_pt); //save only jets passing jid
        jets.SetJetSelection("ak8", skim_jet_ak8_pt, skim_jet_ak8_eta, false); //save all AK8 jets
        //jets.FillCommon(idx_alloverlapjets, jet_corrector_pfL1FastJetL2L3,btagprob_data,btagprob_mc,btagprob_heavy_UP, btagprob_heavy_DN, btagprob_light_UP,btagprob_light_DN,btagprob_FS_UP,btagprob_FS_DN,jet_overlep1_idx, jet_overlep2_idx,applyJECfromFile,jetcorr_uncertainty,JES_type, applyBtagSFs, isFastsim);
        jets.FillCommon(idx_alloverlapjets, jet_corrector_pfL1FastJetL2L3,btagprob_data,btagprob_mc,btagprob_heavy_UP, btagprob_heavy_DN, btagprob_light_UP,btagprob_light_DN,btagprob_FS_UP,btagprob_FS_DN,loosebtagprob_data,loosebtagprob_mc,loosebtagprob_heavy_UP, loosebtagprob_heavy_DN, loosebtagprob_light_UP,loosebtagprob_light_DN,loosebtagprob_FS_UP,loosebtagprob_FS_DN,tightbtagprob_data,tightbtagprob_mc,tightbtagprob_heavy_UP, tightbtagprob_heavy_DN, tightbtagprob_light_UP,tightbtagprob_light_DN,tightbtagprob_FS_UP,tightbtagprob_FS_DN,jet_overlep1_idx, jet_overlep2_idx,applyJECfromFile,jetcorr_uncertainty,JES_type, applyBtagSFs, isFastsim);
        jets.FillAK8Jets(applyAK8JECfromFile, ak8jet_corrector_pfL1FastJetL2L3, ak8jetcorr_uncertainty, JES_type);

        if(applyJECfromFile) {//if no JEC file is defined - code cannot do up/down variations
          //JEC up
          jet_overlep1_idx = -9999;
          jet_overlep2_idx = -9999;
          if(nVetoLeptons>0) jet_overlep1_idx = getOverlappingJetIndex(lep1.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,1,isFastsim);  //don't care about jid
          if(nVetoLeptons>1) jet_overlep2_idx = getOverlappingJetIndex(lep2.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,1,isFastsim);  //don't care about jid

          // Jets and b-tag variables feeding the index for the jet overlapping the selected leptons
          jets_jup.SetJetSelection("ak4", skim_jet_pt, skim_jet_eta, true); //save only jets passing jid
          jets_jup.SetJetSelection("ak8", skim_jet_ak8_pt, skim_jet_ak8_eta, true); //save only jets passing jid
          //jets_jup.FillCommon(idx_alloverlapjets_jup, jet_corrector_pfL1FastJetL2L3,btagprob_data_jup,btagprob_mc_jup,btagprob_heavy_UP_jup, btagprob_heavy_DN_jup, btagprob_light_UP_jup,btagprob_light_DN_jup,btagprob_FS_UP_jup,btagprob_FS_DN_jup,jet_overlep1_idx, jet_overlep2_idx,true,jetcorr_uncertainty_sys,1, false, isFastsim);
          jets_jup.FillCommon(idx_alloverlapjets_jup, jet_corrector_pfL1FastJetL2L3, btagprob_data_jup, btagprob_mc_jup, btagprob_heavy_UP_jup, btagprob_heavy_DN_jup, btagprob_light_UP_jup,btagprob_light_DN_jup,btagprob_FS_UP_jup,btagprob_FS_DN_jup,loosebtagprob_data_jup,loosebtagprob_mc_jup,loosebtagprob_heavy_UP_jup, loosebtagprob_heavy_DN_jup, loosebtagprob_light_UP_jup,loosebtagprob_light_DN_jup,loosebtagprob_FS_UP_jup,loosebtagprob_FS_DN_jup,tightbtagprob_data_jup,tightbtagprob_mc_jup,tightbtagprob_heavy_UP_jup, tightbtagprob_heavy_DN_jup, tightbtagprob_light_UP_jup,tightbtagprob_light_DN_jup,tightbtagprob_FS_UP_jup,tightbtagprob_FS_DN_jup,jet_overlep1_idx, jet_overlep2_idx, true, jetcorr_uncertainty_sys, 1, false, isFastsim);
          jets_jup.FillAK8Jets(applyAK8JECfromFile, ak8jet_corrector_pfL1FastJetL2L3, ak8jetcorr_uncertainty_sys, 1);

          //JEC down
          jet_overlep1_idx = -9999;
          jet_overlep2_idx = -9999;
          if(nVetoLeptons>0) jet_overlep1_idx = getOverlappingJetIndex(lep1.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,-1,isFastsim);  //don't care about jid
          if(nVetoLeptons>1) jet_overlep2_idx = getOverlappingJetIndex(lep2.p4, pfjets_p4(), 0.4, skim_jet_pt, skim_jet_eta, false,jet_corrector_pfL1FastJetL2L3,true,jetcorr_uncertainty_sys,-1,isFastsim);  //don't care about jid

          // Jets and b-tag variables feeding the index for the jet overlapping the selected leptons
          jets_jdown.SetJetSelection("ak4", skim_jet_pt, skim_jet_eta, true); //save only jets passing jid
          jets_jdown.SetJetSelection("ak8", skim_jet_ak8_pt, skim_jet_ak8_eta, true); //save only jets passing jid
          //jets_jdown.FillCommon(idx_alloverlapjets_jdown, jet_corrector_pfL1FastJetL2L3,btagprob_data_jdown,btagprob_mc_jdown,btagprob_heavy_UP_jdown, btagprob_heavy_DN_jdown, btagprob_light_UP_jdown,btagprob_light_DN_jdown,btagprob_FS_UP_jdown,btagprob_FS_DN_jdown,jet_overlep1_idx, jet_overlep2_idx,true,jetcorr_uncertainty_sys,-1, false, isFastsim);
          jets_jdown.FillCommon(idx_alloverlapjets_jdown, jet_corrector_pfL1FastJetL2L3,btagprob_data_jdown,btagprob_mc_jdown,btagprob_heavy_UP_jdown, btagprob_heavy_DN_jdown, btagprob_light_UP_jdown,btagprob_light_DN_jdown,btagprob_FS_UP_jdown,btagprob_FS_DN_jdown,loosebtagprob_data_jdown,loosebtagprob_mc_jdown,loosebtagprob_heavy_UP_jdown, loosebtagprob_heavy_DN_jdown, loosebtagprob_light_UP_jdown,loosebtagprob_light_DN_jdown,loosebtagprob_FS_UP_jdown,loosebtagprob_FS_DN_jdown,tightbtagprob_data_jdown,tightbtagprob_mc_jdown,tightbtagprob_heavy_UP_jdown, tightbtagprob_heavy_DN_jdown, tightbtagprob_light_UP_jdown,tightbtagprob_light_DN_jdown,tightbtagprob_FS_UP_jdown, tightbtagprob_FS_DN_jdown, jet_overlep1_idx, jet_overlep2_idx, true, jetcorr_uncertainty_sys, -1, false, isFastsim);
          jets_jdown.FillAK8Jets(applyAK8JECfromFile, ak8jet_corrector_pfL1FastJetL2L3, ak8jetcorr_uncertainty_sys, -1);
        }

        bool isbadmuonjet = false;
        for(unsigned int jdx = 0; jdx<jets.ak4pfjets_p4.size(); ++jdx){
          jets.dphi_ak4pfjet_met[jdx] = getdphi(jets.ak4pfjets_p4[jdx].Phi(),StopEvt.pfmet_phi);
          if(jets.ak4pfjets_p4[jdx].Pt()>200 && jets.dphi_ak4pfjet_met[jdx]>(TMath::Pi()-0.4) && jets.ak4pfjets_muf[jdx]>0.5) isbadmuonjet = true;
        }
        StopEvt.filt_jetWithBadMuon = !isbadmuonjet;
        isbadmuonjet = false;
        for(unsigned int jdx = 0; jdx<jets_jup.ak4pfjets_p4.size(); ++jdx){
          jets_jup.dphi_ak4pfjet_met[jdx] = getdphi(jets_jup.ak4pfjets_p4[jdx].Phi(),StopEvt.pfmet_phi);
          if(jets_jup.ak4pfjets_p4[jdx].Pt()>200 && jets_jup.dphi_ak4pfjet_met[jdx]>(TMath::Pi()-0.4) && jets_jup.ak4pfjets_muf[jdx]>0.5) isbadmuonjet = true;
        }
        StopEvt.filt_jetWithBadMuon_jup = !isbadmuonjet;
        isbadmuonjet = false;
        for(unsigned int jdx = 0; jdx<jets_jdown.ak4pfjets_p4.size(); ++jdx){
          jets_jdown.dphi_ak4pfjet_met[jdx] = getdphi(jets_jdown.ak4pfjets_p4[jdx].Phi(),StopEvt.pfmet_phi);
          if(jets_jdown.ak4pfjets_p4[jdx].Pt()>200 && jets_jdown.dphi_ak4pfjet_met[jdx]>(TMath::Pi()-0.4) && jets_jdown.ak4pfjets_muf[jdx]>0.5) isbadmuonjet = true;
        }
        StopEvt.filt_jetWithBadMuon_jdown = !isbadmuonjet;
      }

      StopEvt.weight_L1prefire    = 1;
      StopEvt.weight_L1prefire_UP = 1;
      StopEvt.weight_L1prefire_DN = 1;
      if (!evt_isRealData()){
        if (gconf.year == 2016 || gconf.year == 2017)
          std::tie(StopEvt.weight_L1prefire, StopEvt.weight_L1prefire_UP, StopEvt.weight_L1prefire_DN) = getPrefireInfo(gconf.year);

        // Input should have pt > 30, |eta| < 2.4, PF Loose ID, leptons removed
        int NISRjets = get_nisrMatch(jets.ak4pfjets_p4);
        float unc_ISRnjets = get_isrUnc(NISRjets, gconf.year);

        StopEvt.weight_ISRnjets = get_isrWeight(NISRjets, gconf.year);
        StopEvt.weight_ISRnjets_UP = StopEvt.weight_ISRnjets + unc_ISRnjets;
        StopEvt.weight_ISRnjets_DN = StopEvt.weight_ISRnjets - unc_ISRnjets;
        StopEvt.NISRjets = NISRjets;
        StopEvt.NnonISRjets = jets.ngoodjets - NISRjets;

        counterhist->Fill(25,StopEvt.weight_ISRnjets);
        counterhist->Fill(26,StopEvt.weight_ISRnjets_UP);
        counterhist->Fill(27,StopEvt.weight_ISRnjets_DN);
        if(isSignalFromFileName){
          counterhistSig->Fill(mStop,mLSP,24,StopEvt.weight_ISRnjets);
          counterhistSig->Fill(mStop,mLSP,25,StopEvt.weight_ISRnjets_UP);
          counterhistSig->Fill(mStop,mLSP,26,StopEvt.weight_ISRnjets_DN);
        }
      }

      // SAVE B TAGGING SF
      if (!evt_isRealData() && applyBtagSFs) {
        StopEvt.weight_btagsf = btagprob_data / btagprob_mc;
        StopEvt.weight_btagsf_heavy_UP = btagprob_heavy_UP/btagprob_mc;
        StopEvt.weight_btagsf_light_UP = btagprob_light_UP/btagprob_mc;
        StopEvt.weight_btagsf_heavy_DN = btagprob_heavy_DN/btagprob_mc;
        StopEvt.weight_btagsf_light_DN = btagprob_light_DN/btagprob_mc;
        if(isFastsim){
          StopEvt.weight_btagsf_fastsim_UP = btagprob_FS_UP/btagprob_mc;
          StopEvt.weight_btagsf_fastsim_DN = btagprob_FS_DN/btagprob_mc;
        }
        StopEvt.weight_tightbtagsf = tightbtagprob_data / tightbtagprob_mc;
        StopEvt.weight_tightbtagsf_heavy_UP = tightbtagprob_heavy_UP/tightbtagprob_mc;
        StopEvt.weight_tightbtagsf_light_UP = tightbtagprob_light_UP/tightbtagprob_mc;
        StopEvt.weight_tightbtagsf_heavy_DN = tightbtagprob_heavy_DN/tightbtagprob_mc;
        StopEvt.weight_tightbtagsf_light_DN = tightbtagprob_light_DN/tightbtagprob_mc;
        if(isFastsim){
          StopEvt.weight_tightbtagsf_fastsim_UP = tightbtagprob_FS_UP/tightbtagprob_mc;
          StopEvt.weight_tightbtagsf_fastsim_DN = tightbtagprob_FS_DN/tightbtagprob_mc;
        }
        StopEvt.weight_loosebtagsf = loosebtagprob_data / loosebtagprob_mc;
        StopEvt.weight_loosebtagsf_heavy_UP = loosebtagprob_heavy_UP/loosebtagprob_mc;
        StopEvt.weight_loosebtagsf_light_UP = loosebtagprob_light_UP/loosebtagprob_mc;
        StopEvt.weight_loosebtagsf_heavy_DN = loosebtagprob_heavy_DN/loosebtagprob_mc;
        StopEvt.weight_loosebtagsf_light_DN = loosebtagprob_light_DN/loosebtagprob_mc;
        if(isFastsim){
          StopEvt.weight_loosebtagsf_fastsim_UP = loosebtagprob_FS_UP/loosebtagprob_mc;
          StopEvt.weight_loosebtagsf_fastsim_DN = loosebtagprob_FS_DN/loosebtagprob_mc;
        }
        if (isnan(StopEvt.weight_btagsf) || isnan(StopEvt.weight_tightbtagsf) || isnan(StopEvt.weight_loosebtagsf)) {
          cout << "[looper] btagsf is NaN!! btagprob_data = " << btagprob_data << ", btagprob_mc = " << btagprob_mc << endl;
          StopEvt.weight_btagsf = 0;
          continue;
        }

      }
      // save the sum of weights for normalization offline to n-babies.
      // comment: this has to go before the skim_nBJets - else you create a bias
      if(!evt_isRealData() && applyBtagSFs) {
        counterhist->Fill(14,StopEvt.weight_btagsf);
        counterhist->Fill(15,StopEvt.weight_btagsf_heavy_UP);
        counterhist->Fill(16,StopEvt.weight_btagsf_light_UP);
        counterhist->Fill(17,StopEvt.weight_btagsf_heavy_DN);
        counterhist->Fill(18,StopEvt.weight_btagsf_light_DN);
        counterhist->Fill(37,StopEvt.weight_tightbtagsf);
        counterhist->Fill(38,StopEvt.weight_tightbtagsf_heavy_UP);
        counterhist->Fill(39,StopEvt.weight_tightbtagsf_light_UP);
        counterhist->Fill(40,StopEvt.weight_tightbtagsf_heavy_DN);
        counterhist->Fill(41,StopEvt.weight_tightbtagsf_light_DN);
        counterhist->Fill(44,StopEvt.weight_loosebtagsf);
        counterhist->Fill(45,StopEvt.weight_loosebtagsf_heavy_UP);
        counterhist->Fill(46,StopEvt.weight_loosebtagsf_light_UP);
        counterhist->Fill(47,StopEvt.weight_loosebtagsf_heavy_DN);
        counterhist->Fill(48,StopEvt.weight_loosebtagsf_light_DN);
        if(isFastsim){
          counterhist->Fill(23,StopEvt.weight_btagsf_fastsim_UP);
          counterhist->Fill(24,StopEvt.weight_btagsf_fastsim_DN);
          counterhist->Fill(42,StopEvt.weight_tightbtagsf_fastsim_UP);
          counterhist->Fill(43,StopEvt.weight_tightbtagsf_fastsim_DN);
          counterhist->Fill(49,StopEvt.weight_loosebtagsf_fastsim_UP);
          counterhist->Fill(50,StopEvt.weight_loosebtagsf_fastsim_DN);
        }
      }
      if(isSignalFromFileName && !evt_isRealData() && applyBtagSFs){
        counterhistSig->Fill(mStop,mLSP,14,StopEvt.weight_btagsf);
        counterhistSig->Fill(mStop,mLSP,15,StopEvt.weight_btagsf_heavy_UP);
        counterhistSig->Fill(mStop,mLSP,16,StopEvt.weight_btagsf_light_UP);
        counterhistSig->Fill(mStop,mLSP,17,StopEvt.weight_btagsf_heavy_DN);
        counterhistSig->Fill(mStop,mLSP,18,StopEvt.weight_btagsf_light_DN);
        counterhistSig->Fill(mStop,mLSP,37,StopEvt.weight_tightbtagsf);
        counterhistSig->Fill(mStop,mLSP,38,StopEvt.weight_tightbtagsf_heavy_UP);
        counterhistSig->Fill(mStop,mLSP,39,StopEvt.weight_tightbtagsf_light_UP);
        counterhistSig->Fill(mStop,mLSP,40,StopEvt.weight_tightbtagsf_heavy_DN);
        counterhistSig->Fill(mStop,mLSP,41,StopEvt.weight_tightbtagsf_light_DN);
        counterhistSig->Fill(mStop,mLSP,44,StopEvt.weight_loosebtagsf);
        counterhistSig->Fill(mStop,mLSP,45,StopEvt.weight_loosebtagsf_heavy_UP);
        counterhistSig->Fill(mStop,mLSP,46,StopEvt.weight_loosebtagsf_light_UP);
        counterhistSig->Fill(mStop,mLSP,47,StopEvt.weight_loosebtagsf_heavy_DN);
        counterhistSig->Fill(mStop,mLSP,48,StopEvt.weight_loosebtagsf_light_DN);
        if(isFastsim){
          counterhistSig->Fill(mStop,mLSP,22,StopEvt.weight_btagsf_fastsim_UP);
          counterhistSig->Fill(mStop,mLSP,23,StopEvt.weight_btagsf_fastsim_DN);
          counterhistSig->Fill(mStop,mLSP,42,StopEvt.weight_tightbtagsf_fastsim_UP);
          counterhistSig->Fill(mStop,mLSP,43,StopEvt.weight_tightbtagsf_fastsim_DN);
          counterhistSig->Fill(mStop,mLSP,49,StopEvt.weight_loosebtagsf_fastsim_UP);
          counterhistSig->Fill(mStop,mLSP,50,StopEvt.weight_loosebtagsf_fastsim_DN);
        }
      }
      //
      // met Cut
      //
      /*
        if(StopEvt.pfmet < skim_met) continue;
        nEvents_pass_skim_met++;
      */

      if(!(jets.ngoodjets >= skim_nJets) && !(jets_jup.ngoodjets >= skim_nJets) && !(jets_jdown.ngoodjets >= skim_nJets)) continue;
      nEvents_pass_skim_nJets++;
      if(!(jets.ngoodbtags >= skim_nBJets) && !(jets_jup.ngoodbtags >= skim_nBJets) && !(jets_jdown.ngoodbtags >= skim_nBJets)) continue;
      nEvents_pass_skim_nBJets++;

      if(!runTopCandTreeMaker){
        if(nGoodLeptons < skim_nGoodLep) continue;
        nEvents_pass_skim_nGoodLep++;
      }

      // FastSim filter, Nominal Jets
      bool fastsimfilt = false;
      for(unsigned int jix = 0; jix<jets.ak4pfjets_p4.size();++jix){
        if(jets.ak4pfjets_p4[jix].Pt()<30) continue;
        if(fabs(jets.ak4pfjets_p4[jix].Eta())>2.4) continue;
        bool isgenmatch = false;
        for(unsigned int gix = 0; gix<jets.ak4genjets_p4.size();++gix){
          if(dRbetweenVectors(jets.ak4genjets_p4[gix],jets.ak4pfjets_p4[jix])<0.3) { isgenmatch = true; break; }
        }
        if(isgenmatch) continue;
        if(jets.ak4pfjets_chf[jix]>0.1) continue;
        fastsimfilt = true;
        break;
      }
      StopEvt.filt_fastsimjets = !fastsimfilt;

      // FastSim filter, JESup Jets
      bool fastsimfilt_jup = false;
      for(unsigned int jix = 0; jix<jets_jup.ak4pfjets_p4.size();++jix){
        if(jets_jup.ak4pfjets_p4[jix].Pt()<30) continue;
        if(fabs(jets_jup.ak4pfjets_p4[jix].Eta())>2.4) continue;
        bool isgenmatch = false;
        for(unsigned int gix = 0; gix<jets_jup.ak4genjets_p4.size();++gix){
          if(dRbetweenVectors(jets_jup.ak4genjets_p4[gix],jets_jup.ak4pfjets_p4[jix])<0.3) { isgenmatch = true; break; }
        }
        if(isgenmatch) continue;
        if(jets_jup.ak4pfjets_chf[jix]>0.1) continue;
        fastsimfilt_jup = true;
        break;
      }
      StopEvt.filt_fastsimjets_jup = !fastsimfilt_jup;

      // FastSim filter, JESdn Jets
      bool fastsimfilt_jdown = false;
      for(unsigned int jix = 0; jix<jets_jdown.ak4pfjets_p4.size();++jix){
        if(jets_jdown.ak4pfjets_p4[jix].Pt()<30) continue;
        if(fabs(jets_jdown.ak4pfjets_p4[jix].Eta())>2.4) continue;
        bool isgenmatch = false;
        for(unsigned int gix = 0; gix<jets_jdown.ak4genjets_p4.size();++gix){
          if(dRbetweenVectors(jets_jdown.ak4genjets_p4[gix],jets_jdown.ak4pfjets_p4[jix])<0.3) { isgenmatch = true; break; }
        }
        if(isgenmatch) continue;
        if(jets_jdown.ak4pfjets_chf[jix]>0.1) continue;
        fastsimfilt_jdown = true;
        break;
      }
      StopEvt.filt_fastsimjets_jdown = !fastsimfilt_jdown;
      //
      // Photon Selection
      //
      ph.SetPhotonSelection(skim_ph_pt, skim_ph_eta);
      ph.FillCommon();
      StopEvt.nPhotons = ph.p4.size();
      if(StopEvt.nPhotons < skim_nPhotons) continue;
      int leadph = -1;//use this in case we have a wide photon selection (like loose id), but want to use specific photon
      for(unsigned int i = 0; i<ph.p4.size(); ++i){
        int overlapping_jet = getOverlappingJetIndex(ph.p4.at(i), jets.ak4pfjets_p4, 0.4, skim_jet_pt, skim_jet_eta,false,jet_corrector_pfL1FastJetL2L3,applyJECfromFile,jetcorr_uncertainty,JES_type,isFastsim);
        ph.overlapJetId.at(i) = overlapping_jet;
        if(leadph!=-1 && ph.p4.at(i).Pt()<ph.p4.at(leadph).Pt()) continue;
        if(StopEvt.ngoodleps>0 && ROOT::Math::VectorUtil::DeltaR(ph.p4.at(i), lep1.p4)<0.2) continue;
        if(StopEvt.ngoodleps>1 && ROOT::Math::VectorUtil::DeltaR(ph.p4.at(i), lep2.p4)<0.2) continue;
        leadph = i;
      }
      StopEvt.ph_selectedidx = leadph;

      //
      // Event Variables
      //

      // MET & Leptons
      if(nVetoLeptons>0) StopEvt.mt_met_lep = calculateMt(lep1.p4, StopEvt.pfmet, StopEvt.pfmet_phi);
      if(applyJECfromFile){
        if(nVetoLeptons>0) StopEvt.mt_met_lep_jup = calculateMt(lep1.p4, StopEvt.pfmet_jup, StopEvt.pfmet_phi_jup);
        if(nVetoLeptons>0) StopEvt.mt_met_lep_jdown = calculateMt(lep1.p4, StopEvt.pfmet_jdown, StopEvt.pfmet_phi_jdown);
      }
      if(nVetoLeptons>1) StopEvt.mt_met_lep2 = calculateMt(lep2.p4, StopEvt.pfmet, StopEvt.pfmet_phi);
      if(nVetoLeptons>0) StopEvt.dphi_Wlep = DPhi_W_lep(StopEvt.pfmet, StopEvt.pfmet_phi, lep1.p4);
      if(jets.ak4pfjets_p4.size()> 0) StopEvt.MET_over_sqrtHT = StopEvt.pfmet/TMath::Sqrt(jets.ak4_HT);

      StopEvt.ak4pfjets_rho = evt_fixgridfastjet_all_rho();

      vector<int> jetIndexSortedBdisc;
      // Sorted indexes with analysis pt cut, should have enough by requiring ngoodjets >= 2
      if(isFastsim) jetIndexSortedBdisc = JetUtil::JetIndexBdiscSorted(jets.ak4pfjets_deepCSV, jets.ak4pfjets_p4, jets.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, false);
      else jetIndexSortedBdisc = JetUtil::JetIndexBdiscSorted(jets.ak4pfjets_deepCSV, jets.ak4pfjets_p4, jets.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, true);
      vector<LorentzVector> mybjets; vector<LorentzVector> myaddjets;
      for(unsigned int idx = 0; idx<jetIndexSortedBdisc.size(); ++idx){
        if(jets.ak4pfjets_passMEDbtag.at(jetIndexSortedBdisc[idx])==true) mybjets.push_back(jets.ak4pfjets_p4.at(jetIndexSortedBdisc[idx]) );
        else if(mybjets.size()<=1 && (mybjets.size()+myaddjets.size())<3) myaddjets.push_back(jets.ak4pfjets_p4.at(jetIndexSortedBdisc[idx]) );
      }

      vector<int> jetIndexSortedBdisc_jup;
      vector<int> jetIndexSortedBdisc_jdown;
      vector<LorentzVector> mybjets_jup; vector<LorentzVector> myaddjets_jup;
      vector<LorentzVector> mybjets_jdown; vector<LorentzVector> myaddjets_jdown;
      if(applyJECfromFile){
        if(isFastsim) jetIndexSortedBdisc_jup = JetUtil::JetIndexBdiscSorted(jets_jup.ak4pfjets_deepCSV, jets_jup.ak4pfjets_p4, jets_jup.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, false);
        else jetIndexSortedBdisc_jup = JetUtil::JetIndexBdiscSorted(jets_jup.ak4pfjets_deepCSV, jets_jup.ak4pfjets_p4, jets_jup.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, true);
        for(unsigned int idx = 0; idx<jetIndexSortedBdisc_jup.size(); ++idx){
          if(jets_jup.ak4pfjets_passMEDbtag.at(jetIndexSortedBdisc_jup[idx])==true) mybjets_jup.push_back(jets_jup.ak4pfjets_p4.at(jetIndexSortedBdisc_jup[idx]) );
          else if(mybjets_jup.size()<=1 && (mybjets_jup.size()+myaddjets_jup.size())<3) myaddjets_jup.push_back(jets_jup.ak4pfjets_p4.at(jetIndexSortedBdisc_jup[idx]) );
        }

        if(isFastsim) jetIndexSortedBdisc_jdown = JetUtil::JetIndexBdiscSorted(jets_jdown.ak4pfjets_deepCSV, jets_jdown.ak4pfjets_p4, jets_jdown.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, false);
        else jetIndexSortedBdisc_jdown = JetUtil::JetIndexBdiscSorted(jets_jdown.ak4pfjets_deepCSV, jets_jdown.ak4pfjets_p4, jets_jdown.ak4pfjets_tight_pfid, analysis_bjet_pt, skim_jet_eta, true);
        for(unsigned int idx = 0; idx<jetIndexSortedBdisc_jdown.size(); ++idx){
          if(jets_jdown.ak4pfjets_passMEDbtag.at(jetIndexSortedBdisc_jdown[idx])==true) mybjets_jdown.push_back(jets_jdown.ak4pfjets_p4.at(jetIndexSortedBdisc_jdown[idx]) );
          else if(mybjets_jdown.size()<=1 && (mybjets_jdown.size()+myaddjets_jdown.size())<3) myaddjets_jdown.push_back(jets_jdown.ak4pfjets_p4.at(jetIndexSortedBdisc_jdown[idx]) );
        }
      }
      // looks like all the following variables need jets to be calculated.
      //   add protection for skim settings of njets<2
      vector<float> dummy_sigma; dummy_sigma.clear(); //move outside of if-clause to be able to copy for photon selection
      for (size_t idx = 0; idx < jets.ak4pfjets_p4.size(); ++idx){
        dummy_sigma.push_back(0.1);
      }

      // MT2(ll) <-- to avoid overlap with stop-2l signal region
      if(nVetoLeptons>1) {
        StopEvt.MT2_ll = CalcMT2_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,lep2.p4,false,0);
        StopEvt.MT2_ll_jup = CalcMT2_(StopEvt.pfmet_jup,StopEvt.pfmet_phi_jup,lep1.p4,lep2.p4,false,0);
        StopEvt.MT2_ll_jdown = CalcMT2_(StopEvt.pfmet_jdown,StopEvt.pfmet_phi_jdown,lep1.p4,lep2.p4,false,0);
      }

      if(jets.ngoodjets>1){

        // DR(lep, leadB) with medium discriminator
        if(nVetoLeptons>0) StopEvt.dR_lep_leadb = dRbetweenVectors(jets.ak4pfjets_leadMEDbjet_p4, lep1.p4);
        if(nVetoLeptons>1) StopEvt.dR_lep2_leadb = dRbetweenVectors(jets.ak4pfjets_leadMEDbjet_p4, lep2.p4);
        // Hadronic Chi2
        StopEvt.hadronic_top_chi2 = calculateChi2(jets.ak4pfjets_p4, dummy_sigma, jets.ak4pfjets_passMEDbtag);
        // Jets & MET
        StopEvt.mindphi_met_j1_j2 =  getMinDphi(StopEvt.pfmet_phi,jets.ak4pfjets_p4.at(0),jets.ak4pfjets_p4.at(1));
        // MT2W
        if(nVetoLeptons>0) StopEvt.MT2W = CalcMT2W_(mybjets,myaddjets,lep1.p4,StopEvt.pfmet, StopEvt.pfmet_phi);
        if(nVetoLeptons>1) StopEvt.MT2W_lep2 = CalcMT2W_(mybjets,myaddjets,lep2.p4,StopEvt.pfmet, StopEvt.pfmet_phi);
        // Topness
        if(nVetoLeptons>0) StopEvt.topness = CalcTopness_(0,StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets);
        if(nVetoLeptons>1) StopEvt.topness_lep2 = CalcTopness_(0,StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets);
        if(nVetoLeptons>0) StopEvt.topnessMod = CalcTopness_(1,StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets);
        if(nVetoLeptons>1) StopEvt.topnessMod_lep2 = CalcTopness_(1,StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets);
        // MT2(lb,b)
        if(nVetoLeptons>0) StopEvt.MT2_lb_b_mass = CalcMT2_lb_b_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets,0,true);
        if(nVetoLeptons>0) StopEvt.MT2_lb_b = CalcMT2_lb_b_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets,0,false);
        if(nVetoLeptons>1) StopEvt.MT2_lb_b_mass_lep2 = CalcMT2_lb_b_(StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets,0,true);
        if(nVetoLeptons>1) StopEvt.MT2_lb_b_lep2 = CalcMT2_lb_b_(StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets,0,false);
        // MT2(lb,bqq)
        if(nVetoLeptons>0) StopEvt.MT2_lb_bqq_mass = CalcMT2_lb_bqq_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,true);
        if(nVetoLeptons>0) StopEvt.MT2_lb_bqq = CalcMT2_lb_bqq_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,false);
        if(nVetoLeptons>1) StopEvt.MT2_lb_bqq_mass_lep2 = CalcMT2_lb_bqq_(StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,true);
        if(nVetoLeptons>1) StopEvt.MT2_lb_bqq_lep2 = CalcMT2_lb_bqq_(StopEvt.pfmet,StopEvt.pfmet_phi,lep2.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,false);
      }

      if(jets_jup.ngoodjets>1){
        // Jets & MET
        StopEvt.mindphi_met_j1_j2_jup =  getMinDphi(StopEvt.pfmet_phi_jup,jets_jup.ak4pfjets_p4.at(0),jets_jup.ak4pfjets_p4.at(1));
        // MT2W
        if(nVetoLeptons>0) StopEvt.MT2W_jup = CalcMT2W_(mybjets_jup,myaddjets_jup,lep1.p4,StopEvt.pfmet_jup, StopEvt.pfmet_phi_jup);
        // ModTopness
        if(nVetoLeptons>0) StopEvt.topnessMod_jup = CalcTopness_(1,StopEvt.pfmet_jup,StopEvt.pfmet_phi_jup,lep1.p4,mybjets_jup,myaddjets_jup);
      }

      if(jets_jdown.ngoodjets>1){
        // Jets & MET
        StopEvt.mindphi_met_j1_j2_jdown =  getMinDphi(StopEvt.pfmet_phi_jdown,jets_jdown.ak4pfjets_p4.at(0),jets_jdown.ak4pfjets_p4.at(1));
        // MT2W
        if(nVetoLeptons>0) StopEvt.MT2W_jdown = CalcMT2W_(mybjets_jdown,myaddjets_jdown,lep1.p4,StopEvt.pfmet_jdown, StopEvt.pfmet_phi_jdown);
        // ModTopness
        if(nVetoLeptons>0) StopEvt.topnessMod_jdown = CalcTopness_(1,StopEvt.pfmet_jdown,StopEvt.pfmet_phi_jdown,lep1.p4,mybjets_jdown,myaddjets_jdown);
      }


      vector<pair<float, int> > rankminDR;
      vector<pair<float, int> > rankminDR_jup;
      vector<pair<float, int> > rankminDR_jdown;
      vector<pair<float, int> > rankmaxDPhi;
      vector<pair<float, int> > rankminDR_lep2;
      vector<pair<float, int> > rankmaxDPhi_lep2;
      for (unsigned int idx = 0; idx < jets.ak4pfjets_p4.size(); ++idx){
        if(nVetoLeptons==0) continue;
        pair<float, int> mypair;
        mypair.second = idx;
        mypair.first = getdphi(jets.ak4pfjets_p4.at(idx).Phi(),lep1.p4.Phi());
        rankmaxDPhi.push_back(mypair);
        mypair.first = dRbetweenVectors(jets.ak4pfjets_p4.at(idx),lep1.p4);
        rankminDR.push_back(mypair);
        if(nVetoLeptons<=1) continue;
        mypair.first = getdphi(jets.ak4pfjets_p4.at(idx).Phi(),lep2.p4.Phi());
        rankmaxDPhi_lep2.push_back(mypair);
        mypair.first = dRbetweenVectors(jets.ak4pfjets_p4.at(idx),lep2.p4);
        rankminDR_lep2.push_back(mypair);
      }
      for (unsigned int idx = 0; idx < jets_jup.ak4pfjets_p4.size(); ++idx){
        if(nVetoLeptons==0) continue;
        pair<float, int> mypair;
        mypair.second = idx;
        mypair.first = dRbetweenVectors(jets_jup.ak4pfjets_p4.at(idx),lep1.p4);
        rankminDR_jup.push_back(mypair);
      }
      for (unsigned int idx = 0; idx < jets_jdown.ak4pfjets_p4.size(); ++idx){
        if(nVetoLeptons==0) continue;
        pair<float, int> mypair;
        mypair.second = idx;
        mypair.first = dRbetweenVectors(jets_jdown.ak4pfjets_p4.at(idx),lep1.p4);
        rankminDR_jdown.push_back(mypair);
      }
      sort(rankminDR.begin(),        rankminDR.end(),        CompareIndexValueSmallest);
      sort(rankminDR_jup.begin(),    rankminDR_jup.end(),    CompareIndexValueSmallest);
      sort(rankminDR_jdown.begin(),  rankminDR_jdown.end(),  CompareIndexValueSmallest);
      sort(rankminDR_lep2.begin(),   rankminDR_lep2.end(),   CompareIndexValueSmallest);
      sort(rankmaxDPhi.begin(),      rankmaxDPhi.end(),      CompareIndexValueGreatest);
      sort(rankmaxDPhi_lep2.begin(), rankmaxDPhi_lep2.end(), CompareIndexValueGreatest);

      if(jets.ngoodjets>0){
        for (unsigned int idx = 0; idx < rankminDR.size(); ++idx){
          if(nVetoLeptons==0) continue;
          if(!(jets.ak4pfjets_passMEDbtag.at(rankminDR[idx].second)) ) continue;
          StopEvt.Mlb_closestb = (jets.ak4pfjets_p4.at(rankminDR[idx].second)+lep1.p4).M();
          if(StopEvt.Mlb_closestb<175) {
            StopEvt.weight_analysisbtagsf = StopEvt.weight_btagsf;
            StopEvt.weight_analysisbtagsf_heavy_UP = StopEvt.weight_btagsf_heavy_UP;
            StopEvt.weight_analysisbtagsf_light_UP = StopEvt.weight_btagsf_light_UP;
            StopEvt.weight_analysisbtagsf_heavy_DN = StopEvt.weight_btagsf_heavy_DN;
            StopEvt.weight_analysisbtagsf_light_DN = StopEvt.weight_btagsf_light_DN;
            StopEvt.weight_analysisbtagsf_fastsim_UP = StopEvt.weight_btagsf_fastsim_UP;
            StopEvt.weight_analysisbtagsf_fastsim_DN = StopEvt.weight_btagsf_fastsim_DN;
            jets.nanalysisbtags = jets.ngoodbtags;
            jets_jup.nanalysisbtags = jets_jup.ngoodbtags;
            jets_jdown.nanalysisbtags = jets_jdown.ngoodbtags;
          }
          else  {
            StopEvt.weight_analysisbtagsf = StopEvt.weight_tightbtagsf;
            StopEvt.weight_analysisbtagsf_heavy_UP = StopEvt.weight_tightbtagsf_heavy_UP;
            StopEvt.weight_analysisbtagsf_light_UP = StopEvt.weight_tightbtagsf_light_UP;
            StopEvt.weight_analysisbtagsf_heavy_DN = StopEvt.weight_tightbtagsf_heavy_DN;
            StopEvt.weight_analysisbtagsf_light_DN = StopEvt.weight_tightbtagsf_light_DN;
            StopEvt.weight_analysisbtagsf_fastsim_UP = StopEvt.weight_tightbtagsf_fastsim_UP;
            StopEvt.weight_analysisbtagsf_fastsim_DN = StopEvt.weight_tightbtagsf_fastsim_DN;
            jets.nanalysisbtags = jets.ntightbtags;
            jets_jup.nanalysisbtags = jets_jup.ntightbtags;
            jets_jdown.nanalysisbtags = jets_jdown.ntightbtags;
          }
          break;
        }
        for (unsigned int idx = 0; idx < rankminDR_lep2.size(); ++idx){
          if(nVetoLeptons<=1) continue;
          if(!(jets.ak4pfjets_passMEDbtag.at(rankminDR_lep2[idx].second)) ) continue;
          StopEvt.Mlb_closestb_lep2 = (jets.ak4pfjets_p4.at(rankminDR_lep2[idx].second)+lep2.p4).M();
          break;
        }
        if(nVetoLeptons>0) StopEvt.Mlb_lead_bdiscr = (jets.ak4pfjets_p4.at(jetIndexSortedBdisc[0])+lep1.p4).M();
        if(nVetoLeptons>1) StopEvt.Mlb_lead_bdiscr_lep2 = (jets.ak4pfjets_p4.at(jetIndexSortedBdisc[0])+lep2.p4).M();
        if(rankmaxDPhi.size()>=3) {
          StopEvt.Mjjj = (jets.ak4pfjets_p4.at(rankmaxDPhi[0].second)+jets.ak4pfjets_p4.at(rankmaxDPhi[1].second)+jets.ak4pfjets_p4.at(rankmaxDPhi[2].second)).M();
        }
        if(rankmaxDPhi_lep2.size()>=3) {
          StopEvt.Mjjj_lep2 = (jets.ak4pfjets_p4.at(rankmaxDPhi_lep2[0].second)+jets.ak4pfjets_p4.at(rankmaxDPhi_lep2[1].second)+jets.ak4pfjets_p4.at(rankmaxDPhi_lep2[2].second)).M();
          //StopEvt.Mjjj_lep2 = (jets.ak4pfjets_p4.at(jb21)+jets.ak4pfjets_p4.at(jb22)+jets.ak4pfjets_p4.at(jb23)).M();
        }

      } // end if >0 jets
      if(jets_jup.ngoodjets>0){
        for (unsigned int idx = 0; idx < rankminDR_jup.size(); ++idx){
          if(nVetoLeptons==0) continue;
          if(!(jets_jup.ak4pfjets_passMEDbtag.at(rankminDR_jup[idx].second)) ) continue;
          StopEvt.Mlb_closestb_jup = (jets_jup.ak4pfjets_p4.at(rankminDR_jup[idx].second)+lep1.p4).M();
          break;
        }
        if(nVetoLeptons>0) StopEvt.Mlb_lead_bdiscr_jup = (jets_jup.ak4pfjets_p4.at(jetIndexSortedBdisc_jup[0])+lep1.p4).M();
      }
      if(jets_jdown.ngoodjets>0){
        for (unsigned int idx = 0; idx < rankminDR_jdown.size(); ++idx){
          if(nVetoLeptons==0) continue;
          if(!(jets_jdown.ak4pfjets_passMEDbtag.at(rankminDR_jdown[idx].second)) ) continue;
          StopEvt.Mlb_closestb_jdown = (jets_jdown.ak4pfjets_p4.at(rankminDR_jdown[idx].second)+lep1.p4).M();
          break;
        }
        if(nVetoLeptons>0) StopEvt.Mlb_lead_bdiscr_jdown = (jets_jdown.ak4pfjets_p4.at(jetIndexSortedBdisc_jdown[0])+lep1.p4).M();
      }

      // Fill the tree for top tagger training flat ntuple before rejecting on leptons
      if(runTopCandTreeMaker){
        bool eventHasTop = (StopEvt.dataset.find("TTJets") != string::npos && !StopEvt.is2lep);
        if(eventHasTop || evt %4 == 0){
          topcandTreeMaker->ResetAll();  // be here for safe first
          topcandTreeMaker->AddEventInfo(evt_event(), evt_scale1fb(), evt_pfmet(), numberOfGoodVertices(),
                                         nVetoLeptons, jets.ngoodjets, jets.ngoodbtags, jets.nloosebtags,
                                         StopEvt.mt_met_lep, StopEvt.topnessMod, StopEvt.Mlb_closestb);
          topcandTreeMaker->SetJetVectors(&jets.ak4pfjets_p4, &jets.ak4pfjets_CSV, &jets.ak4pfjets_cvsl,
                                          &jets.ak4pfjets_ptD, &jets.ak4pfjets_axis1, &jets.ak4pfjets_axis2, &jets.ak4pfjets_mult,
                                          &jets.ak4pfjets_deepCSVb, &jets.ak4pfjets_deepCSVc, &jets.ak4pfjets_deepCSVl, &jets.ak4pfjets_deepCSVbb);
          // topcandTreeMaker->SetGenParticleVectors(&gen_qs.p4, &gen_qs.id, &gen_qs.isLastCopy, &gen_qs.motherid, &gen_qs.motheridx);
          topcandTreeMaker->SetGenParticleVectors(&genps_p4(), &genps_id(), &genps_id_mother(), &genps_idx_mother(), &genps_isLastCopy(),
                                                  &pfjets_p4(), &pfjets_partonFlavour());
                                                  // &jets.ak4pfjets_p4, &jets.ak4pfjets_parton_flavor);
          topcandTreeMaker->FillTree();
        }

        if(nGoodLeptons < skim_nGoodLep) continue;
        nEvents_pass_skim_nGoodLep++;
      }

      if(fillZll){
        //
        // Zll Event Variables
        //
        //  first find a Zll
        //  fill only for 2 or three lepton events//Zl2 will have always idx 1(2) for 2l(3l) events
        //  if four lepton events test only leading three leptons
        //  Zll must be always OS, then prefer OF, then prefer Zmass
        //  Zll needs to go before ph, as we recalculate myaddjets, mybjets
        if(nLooseLeptons>=2){
          int Zl1 = -1;
          if(nLooseLeptons==2 && AllLeps[0].id*AllLeps[1].id<0) Zl1 = 0;
          else if(nLooseLeptons>=3){
            if(     nGoodLeptons==1 && AllLeps[1].id*AllLeps[2].id<0) Zl1 = 1;//0 is taken by selection lepton
            else if(nGoodLeptons>=2){//selection lepton can be either 0(lep1) or 1(lep2)
              if(     (AllLeps[0].id*AllLeps[2].id)<0 && (AllLeps[1].id*AllLeps[2].id)>0) Zl1 = 0;
              else if((AllLeps[0].id*AllLeps[2].id)>0 && (AllLeps[1].id*AllLeps[2].id)<0) Zl1 = 1;
              else if((AllLeps[0].id*AllLeps[2].id)<0 && (AllLeps[1].id*AllLeps[2].id)<0){
                if(      (abs(AllLeps[0].id)==abs(AllLeps[2].id)) && !(abs(AllLeps[1].id)==abs(AllLeps[2].id))) Zl1 = 0;
                else if(!(abs(AllLeps[0].id)==abs(AllLeps[2].id)) &&  (abs(AllLeps[1].id)==abs(AllLeps[2].id))) Zl1 = 1;
                else {//Z will be SF/OF for either combination, decide on Zmass
                  if(fabs((AllLeps[0].p4+AllLeps[2].p4).M()-91.)>fabs((AllLeps[1].p4+AllLeps[2].p4).M()-91.)) Zl1 = 1;
                  else Zl1 = 0;
                }
              }
            }
          }//end of Zl1 selection
          if(Zl1>=0){//found a Zll candidate
            int Zl2 = -1;
            if(nLooseLeptons==2) Zl2 = 1;
            else Zl2 = 2;
            StopEvt.Zll_idl1 = AllLeps[Zl1].id;
            StopEvt.Zll_idl2 = AllLeps[Zl2].id;
            StopEvt.Zll_p4l1 = AllLeps[Zl1].p4;
            StopEvt.Zll_p4l2 = AllLeps[Zl2].p4;
            StopEvt.Zll_OS = (AllLeps[Zl1].id*AllLeps[Zl2].id<0);
            StopEvt.Zll_SF = (abs(AllLeps[Zl1].id)==abs(AllLeps[Zl2].id));
            StopEvt.Zll_isZmass = (fabs((AllLeps[Zl1].p4+AllLeps[Zl2].p4).M()-91.)<15);
            StopEvt.Zll_M = (AllLeps[Zl1].p4+AllLeps[Zl2].p4).M();
            StopEvt.Zll_p4 = (AllLeps[Zl1].p4+AllLeps[Zl2].p4);
            if(nLooseLeptons==2) StopEvt.Zll_selLep = 1;//selection lepton (not Zll) is lep1
            else if(Zl1==0) StopEvt.Zll_selLep = 2;//selection lepton (not Zll) is lep2
            else StopEvt.Zll_selLep = 1;//selection lepton (not Zll) is lep1
            //Zll_selLep decides if one has to use Mlb_closestb or Mlb_closestb_lep2 for variables without MET.
            //it also decides if we recalculate mt_met_lep using lep1 or lep2, etc.
            double Zllmetpx = ( StopEvt.pfmet * cos(StopEvt.pfmet_phi) ) + ( StopEvt.Zll_p4.Px() );
            double Zllmetpy = ( StopEvt.pfmet * sin(StopEvt.pfmet_phi) ) + ( StopEvt.Zll_p4.Py() );
            StopEvt.Zll_met     = sqrt(pow(Zllmetpx,2)+pow(Zllmetpy,2) );
            StopEvt.Zll_met_phi = atan2(Zllmetpy,Zllmetpx);
            if(jets.ngoodjets>1) StopEvt.Zll_mindphi_met_j1_j2 =  getMinDphi(StopEvt.Zll_met_phi,jets.ak4pfjets_p4.at(0),jets.ak4pfjets_p4.at(1));
            if(nVetoLeptons>2) StopEvt.Zll_MT2_ll = CalcMT2_(StopEvt.pfmet,StopEvt.pfmet_phi,AllLeps[Zl1].p4,AllLeps[Zl2].p4,false,0);
            if(StopEvt.Zll_selLep == 1){
              StopEvt.Zll_mt_met_lep = calculateMt(lep1.p4, StopEvt.Zll_met, StopEvt.Zll_met_phi);
              StopEvt.Zll_dphi_Wlep = DPhi_W_lep(StopEvt.Zll_met, StopEvt.Zll_met_phi, lep1.p4);
              if(jets.ngoodjets>1){
                StopEvt.Zll_MT2W = CalcMT2W_(mybjets,myaddjets,lep1.p4,StopEvt.Zll_met, StopEvt.Zll_met_phi);
                StopEvt.Zll_topness = CalcTopness_(0,StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets);
                StopEvt.Zll_topnessMod = CalcTopness_(1,StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets);
                StopEvt.Zll_MT2_lb_b_mass = CalcMT2_lb_b_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets,0,true);
                StopEvt.Zll_MT2_lb_b = CalcMT2_lb_b_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets,0,false);
                StopEvt.Zll_MT2_lb_bqq_mass = CalcMT2_lb_bqq_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,true);
                StopEvt.Zll_MT2_lb_bqq = CalcMT2_lb_bqq_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep1.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,false);
              }
            } else {
              StopEvt.Zll_mt_met_lep = calculateMt(lep2.p4, StopEvt.Zll_met, StopEvt.Zll_met_phi);
              StopEvt.Zll_dphi_Wlep = DPhi_W_lep(StopEvt.Zll_met, StopEvt.Zll_met_phi, lep2.p4);
              if(jets.ngoodjets>1){
                StopEvt.Zll_MT2W = CalcMT2W_(mybjets,myaddjets,lep2.p4,StopEvt.Zll_met, StopEvt.Zll_met_phi);
                StopEvt.Zll_topness = CalcTopness_(0,StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets);
                StopEvt.Zll_topnessMod = CalcTopness_(1,StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets);
                StopEvt.Zll_MT2_lb_b_mass = CalcMT2_lb_b_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets,0,true);
                StopEvt.Zll_MT2_lb_b = CalcMT2_lb_b_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets,0,false);
                StopEvt.Zll_MT2_lb_bqq_mass = CalcMT2_lb_bqq_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,true);
                StopEvt.Zll_MT2_lb_bqq = CalcMT2_lb_bqq_(StopEvt.Zll_met, StopEvt.Zll_met_phi,lep2.p4,mybjets,myaddjets,jets.ak4pfjets_p4,0,false);
              }
            }
          }//end of Zll filling
        }//end of Zll
      }

      if(fillPhoton){
        //
        // Photon Event Variables
        //
        if(StopEvt.ph_selectedidx>=0){
          int oljind = ph.overlapJetId.at(StopEvt.ph_selectedidx);
          double phmetpx = ( StopEvt.pfmet * cos(StopEvt.pfmet_phi) ) + ( ph.p4.at(StopEvt.ph_selectedidx).Px() );
          double phmetpy = ( StopEvt.pfmet * sin(StopEvt.pfmet_phi) ) + ( ph.p4.at(StopEvt.ph_selectedidx).Py() );
          StopEvt.ph_met     = sqrt(pow(phmetpx,2)+pow(phmetpy,2) );
          StopEvt.ph_met_phi = atan2(phmetpy,phmetpx);
          StopEvt.ph_ngoodjets = jets.ngoodjets;
          StopEvt.ph_ngoodbtags = jets.ngoodbtags;
          if(oljind>=0){
            StopEvt.ph_ngoodjets = jets.ngoodjets-1;
            if(jets.ak4pfjets_passMEDbtag.at(oljind)==true) StopEvt.ph_ngoodbtags = jets.ngoodbtags-1;
          }
          vector<LorentzVector> jetsp4_phcleaned; jetsp4_phcleaned.clear();//used
          vector<float>         jetsDeepCSV_phcleaned; jetsDeepCSV_phcleaned.clear();
          vector<bool>          jetsbtag_phcleaned; jetsbtag_phcleaned.clear();
          vector<float>         dummy_sigma_phcleaned; dummy_sigma_phcleaned.clear();//used
          int leadbidx = -1;
          float htph(0), htssmph(0), htosmph(0);
          for (unsigned int idx = 0; idx < jets.ak4pfjets_p4.size(); ++idx){
            if((int)idx==oljind) continue;
            jetsp4_phcleaned.push_back(jets.ak4pfjets_p4.at(idx));
            jetsDeepCSV_phcleaned.push_back(jets.ak4pfjets_deepCSV.at(idx));
            jetsbtag_phcleaned.push_back(jets.ak4pfjets_passMEDbtag.at(idx));
            dummy_sigma_phcleaned.push_back(dummy_sigma.at(idx));
            htph += jets.ak4pfjets_pt.at(idx);
            float dPhiM = getdphi(StopEvt.ph_met_phi, jets.ak4pfjets_phi.at(idx) );
            if ( dPhiM  < (TMath::Pi()/2) ) htssmph += jets.ak4pfjets_pt.at(idx);
            else                            htosmph += jets.ak4pfjets_pt.at(idx);
            if(leadbidx==-1 && jets.ak4pfjets_passMEDbtag.at(idx)) leadbidx = idx;//leading bjet photon cleaned
          }
          StopEvt.ph_HT = htph;
          StopEvt.ph_htssm = htssmph;
          StopEvt.ph_htosm = htosmph;
          StopEvt.ph_htratiom   = StopEvt.ph_htssm / (StopEvt.ph_htosm + StopEvt.ph_htssm);
          mybjets.clear(); myaddjets.clear();
          for(unsigned int idx = 0; idx<jetIndexSortedBdisc.size(); ++idx){
            if(jetIndexSortedBdisc[idx]==oljind) continue;
            if(jets.ak4pfjets_passMEDbtag.at(jetIndexSortedBdisc[idx])==true) mybjets.push_back(jets.ak4pfjets_p4.at(jetIndexSortedBdisc[idx]) );
            else if(mybjets.size()<=1 && (mybjets.size()+myaddjets.size())<3) myaddjets.push_back(jets.ak4pfjets_p4.at(jetIndexSortedBdisc[idx]) );
          }
          if(nVetoLeptons>0) {
            StopEvt.ph_mt_met_lep = calculateMt(lep1.p4, StopEvt.ph_met, StopEvt.ph_met_phi);
            StopEvt.ph_dphi_Wlep = DPhi_W_lep(StopEvt.ph_met, StopEvt.ph_met_phi, lep1.p4);
          }
          if(nVetoLeptons>1) StopEvt.ph_MT2_ll = CalcMT2_(StopEvt.pfmet,StopEvt.pfmet_phi,lep1.p4,lep2.p4,false,0);
          if(jetsp4_phcleaned.size()>1){
            if(nVetoLeptons>0) {
              StopEvt.ph_MT2W = CalcMT2W_(mybjets,myaddjets,lep1.p4,StopEvt.ph_met, StopEvt.ph_met_phi);
              StopEvt.ph_topness = CalcTopness_(0,StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets);
              StopEvt.ph_topnessMod = CalcTopness_(1,StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets);
              StopEvt.ph_MT2_lb_b_mass = CalcMT2_lb_b_(StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets,0,true);
              StopEvt.ph_MT2_lb_b = CalcMT2_lb_b_(StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets,0,false);
              StopEvt.ph_MT2_lb_bqq_mass = CalcMT2_lb_bqq_(StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets,jetsp4_phcleaned,0,true);
              StopEvt.ph_MT2_lb_bqq = CalcMT2_lb_bqq_(StopEvt.ph_met, StopEvt.ph_met_phi,lep1.p4,mybjets,myaddjets,jetsp4_phcleaned,0,false);
            }
            StopEvt.ph_hadronic_top_chi2 = calculateChi2(jetsp4_phcleaned, dummy_sigma_phcleaned, jetsbtag_phcleaned);
            StopEvt.ph_mindphi_met_j1_j2 =  getMinDphi(StopEvt.ph_met_phi,jetsp4_phcleaned.at(0),jetsp4_phcleaned.at(1));
          }//at least two jets
          if(jetsp4_phcleaned.size()>0){
            if(nVetoLeptons>0) {
              StopEvt.ph_Mlb_lead_bdiscr = (jets.ak4pfjets_p4.at(jetIndexSortedBdisc[0])+lep1.p4).M();
              if(oljind==jetIndexSortedBdisc[0]) StopEvt.ph_Mlb_lead_bdiscr = (jets.ak4pfjets_p4.at(jetIndexSortedBdisc[1])+lep1.p4).M();//exists as index=0 doesn't count for jetsp4_phcleaned.size()
              if(leadbidx>=0) StopEvt.ph_dR_lep_leadb = dRbetweenVectors(jets.ak4pfjets_p4.at(leadbidx), lep1.p4);
              for (unsigned int idx = 0; idx < rankminDR.size(); ++idx){
                if(rankminDR[idx].second==oljind) continue;
                if(nVetoLeptons==0) continue;
                if(!(jets.ak4pfjets_passMEDbtag.at(rankminDR[idx].second)) ) continue;
                StopEvt.ph_Mlb_closestb = (jets.ak4pfjets_p4.at(rankminDR[idx].second)+lep1.p4).M();
                break;
              }
              LorentzVector threejetsum; threejetsum.SetPxPyPzE(0.,0.,0.,0.);
              int threejetcounter = 0;
              for (unsigned int idx = 0; idx < rankmaxDPhi.size(); ++idx){
                if(rankmaxDPhi[idx].second==oljind) continue;
                threejetsum = threejetsum + jets.ak4pfjets_p4.at(rankmaxDPhi[idx].second);
                ++threejetcounter;
                if(threejetcounter>=3) break;
              }
              if(threejetcounter==3) StopEvt.ph_Mjjj = threejetsum.M();
            }//at least one lepton
          }//at least one jet
        }//end of photon additions
      }
      //
      // Tau Selection
      //
      //std::cout << "[babymaker::looper]: tau  vars" << std::endl;
      int vetotaus=0;
      double tau_pt  = 20.0;
      double tau_eta = 2.3;

      Taus.tau_IDnames = taus_pf_IDnames();

      for(unsigned int iTau = 0; iTau < taus_pf_p4().size(); iTau++){
        if( taus_pf_p4().at(iTau).Pt() < tau_pt ) continue;
        if( fabs(taus_pf_p4().at(iTau).Eta()) > tau_eta ) continue;

        Taus.FillCommon(iTau, tau_pt, tau_eta);
        bool is_vetotau = isVetoTau(iTau, lep1.p4, lep1.charge);  // Moriond17 analysis selection
        Taus.tau_isVetoTau.push_back(is_vetotau);
        bool is_vetotau_v2 = isVetoTau_v2(iTau, lep1.p4, lep1.charge); // Legacy analysis selection: new decay mode
        Taus.tau_isVetoTau_v2.push_back(is_vetotau_v2);

        if (is_vetotau_v2) vetotaus++;
      }

      if(vetotaus<1) StopEvt.PassTauVeto = true;
      else StopEvt.PassTauVeto = false;
      Taus.ngoodtaus = vetotaus;

      //
      // IsoTracks (Charged pfLeptons and pfChargedHadrons)
      //
      if (gconf.cmssw_ver < 94) {
        // Old method for 80X samples when isotrack branches are not available in MiniAOD
        int vetotracks = 0;
        int vetotracks_v2 = 0;
        int vetotracks_v3 = 0;
        for (unsigned int ipf = 0; ipf < pfcands_p4().size(); ipf++) {

          //some selections
          if(pfcands_charge().at(ipf) == 0) continue;
          if(pfcands_p4().at(ipf).pt() < 5) continue;
          if(fabs(pfcands_p4().at(ipf).eta()) > 2.4 ) continue;
          if(fabs(pfcands_dz().at(ipf)) > 0.1) continue;

          //remove everything that is within 0.1 of selected lead and subleading leptons
          if(nVetoLeptons>0){
            if(ROOT::Math::VectorUtil::DeltaR(pfcands_p4().at(ipf), lep1.p4)<0.1) continue;
          }
          if(nVetoLeptons>1){
            if(ROOT::Math::VectorUtil::DeltaR(pfcands_p4().at(ipf), lep2.p4)<0.1) continue;
          }

          Tracks.FillCommon(ipf);

          // 8 TeV Track Isolation Configuration
          if(nVetoLeptons>0){
            if(isVetoTrack(ipf, lep1.p4, lep1.charge)){
              Tracks.isoTracks_isVetoTrack.push_back(true);
              vetotracks++;
            }else Tracks.isoTracks_isVetoTrack.push_back(false);
          }
          else{
            LorentzVector temp( -99.9, -99.9, -99.9, -99.9 );
            if(isVetoTrack(ipf, temp, 0)){
              Tracks.isoTracks_isVetoTrack.push_back(true);
              vetotracks++;
            }else Tracks.isoTracks_isVetoTrack.push_back(false);
          }

          // 13 TeV Track Isolation Configuration, pfLep and pfCH
          if(nVetoLeptons>0){
            if(isVetoTrack_v2(ipf, lep1.p4, lep1.charge)){
              Tracks.isoTracks_isVetoTrack_v2.push_back(true);
              vetotracks_v2++;
            }else Tracks.isoTracks_isVetoTrack_v2.push_back(false);
          }
          else{
            LorentzVector temp( -99.9, -99.9, -99.9, -99.9 );
            if(isVetoTrack_v2(ipf, temp, 0)){
              Tracks.isoTracks_isVetoTrack_v2.push_back(true);
              vetotracks_v2++;
            }else Tracks.isoTracks_isVetoTrack_v2.push_back(false);
          }

          // 13 TeV Track Isolation Configuration, pfCH
          if(nVetoLeptons>0){
            if(isVetoTrack_v3(ipf, lep1.p4, lep1.charge)){
              Tracks.isoTracks_isVetoTrack_v3.push_back(true);
              vetotracks_v3++;
            }else Tracks.isoTracks_isVetoTrack_v3.push_back(false);
          }
          else{
            LorentzVector temp( -99.9, -99.9, -99.9, -99.9 );
            if(isVetoTrack_v3(ipf, temp, 0)){
              Tracks.isoTracks_isVetoTrack_v3.push_back(true);
              vetotracks_v3++;
            }else Tracks.isoTracks_isVetoTrack_v3.push_back(false);
          }

        } // end loop over pfCands

        StopEvt.PassTrackVeto = (vetotracks_v3 < 1);
      } else {
        // Newer method when isotrack branches are available
        int nIsoTracks = 0;
        for (unsigned int itrk = 0; itrk < isotracks_p4().size(); ++itrk) {
          if (!isotracks_isPFCand().at(itrk)) continue;  // only consider pfcandidates
          if (isotracks_p4().at(itrk).pt() < 10) continue;
          if (fabs(isotracks_p4().at(itrk).eta()) > 2.4 ) continue;
          if (isotracks_charge().at(itrk) == 0) continue;
          if (fabs(isotracks_dz().at(itrk)) > 0.1) continue;
          if (isotracks_lepOverlap().at(itrk)) continue;  // should remove all lep overlap, but it didn't, so we need the line below
          if (nVetoLeptons > 0 && utils::isCloseObject(isotracks_p4().at(itrk), lep1.p4, 0.4)) continue;
          if (nVetoLeptons > 1 && utils::isCloseObject(isotracks_p4().at(itrk), lep2.p4, 0.4)) continue;
          if (isotracks_charge().at(itrk) * lep1.charge >= 0) continue; // opposite to lead lepton

          Tracks.FillCommon(itrk, 1);
          // float pfiso = isotracks_pfIso_ch().at(itrk) + isotracks_pfIso_nh().at(itrk) + isotracks_pfIso_em().at(itrk);
          float pfiso = isotracks_pfIso_ch().at(itrk);

          //if not electron or muon
          if (abs(isotracks_particleId().at(itrk))==11 || abs(isotracks_particleId().at(itrk))==13)
            cout << "[looper]>> Find isotrack that is a lepton <-- shouldn't happen." << endl;

          bool isVetoTrack = false;
          if (isotracks_p4().at(itrk).pt() > 60.) {
            if (pfiso < 6.0 ) isVetoTrack = true;
          } else {
            if (pfiso/isotracks_p4().at(itrk).pt() < 0.1) isVetoTrack = true;
          }
          Tracks.isoTracks_isVetoTrack_v3.push_back(isVetoTrack);

          if (isVetoTrack) nIsoTracks++;
        }
        StopEvt.PassTrackVeto = (nIsoTracks < 1);
      }

      if(apply2ndLepVeto){
        if(StopEvt.nvetoleps!=1) continue;
        if(!StopEvt.PassTrackVeto) continue;
        if(!StopEvt.PassTauVeto) continue;
      }
      nEvents_pass_skim_2ndlepVeto++;

      //std::cout << "[babymaker::looper]: updating geninfo for recoleptons" << std::endl;
      // Check that we have the gen leptons matched to reco leptons
      int lep1_match_idx = -99;
      int lep2_match_idx = -99;

      double min_dr_lep1 = 999.9;
      int min_dr_lep1_idx = -99;

      double min_dr_lep2 = 999.9;
      int min_dr_lep2_idx = -99;

      if (!evt_isRealData()){

        if(nVetoLeptons>0){
          for(int iGen=0; iGen<(int)gen_leps.p4.size(); iGen++){
            if( abs(gen_leps.id.at(iGen))==abs(lep1.pdgid) ){
              double temp_dr = ROOT::Math::VectorUtil::DeltaR(gen_leps.p4.at(iGen), lep1.p4);
              if(temp_dr<matched_dr){
                lep1_match_idx=gen_leps.genpsidx.at(iGen);
                break;
              }
              else if(temp_dr<min_dr_lep1){
                min_dr_lep1=temp_dr;
                min_dr_lep1_idx=gen_leps.genpsidx.at(iGen);
              }
            } // end if lep1 id matches genLep id
          } // end loop over gen leps
        }
        if(nVetoLeptons>1){
          for(int iGen=0; iGen<(int)gen_leps.p4.size(); iGen++){
            if( lep1_match_idx == iGen ) continue;
            if( abs(gen_leps.id.at(iGen))==abs(lep2.pdgid) ){
              double temp_dr = ROOT::Math::VectorUtil::DeltaR(gen_leps.p4.at(iGen), lep2.p4);
              if(temp_dr<matched_dr){
                lep2_match_idx=gen_leps.genpsidx.at(iGen);
                break;
              }
              else if(temp_dr<min_dr_lep2){
                min_dr_lep2=temp_dr;
                min_dr_lep2_idx=gen_leps.genpsidx.at(iGen);
              }
            }
          }
        }

        // If lep1 isn't matched to a lepton already stored, then try to find another match
        if( (nVetoLeptons>0 && lep1_match_idx<0) ){
          for(unsigned int genx = 0; genx < genps_p4().size(); genx++){
            if(!genps_isLastCopy().at(genx) ) continue;
            if( abs(genps_id().at(genx))==abs(lep1.pdgid) ){
              double temp_dr = ROOT::Math::VectorUtil::DeltaR(genps_p4().at(genx), lep1.p4);
              if(temp_dr<matched_dr){
                lep1_match_idx=genx;
                gen_leps.FillCommon(genx);
                break;
              }
              else if(temp_dr<min_dr_lep1){
                min_dr_lep1=temp_dr;
                min_dr_lep1_idx=genx;
              }
            }
          }
          // if lep1 is still unmatched, fill with closest match, if possible
          if( lep1_match_idx<0 && min_dr_lep1_idx>0 ) gen_leps.FillCommon(min_dr_lep1_idx);
        }

        // If lep2 isn't matched to a lepton already stored, then try to find another match
        if( (nVetoLeptons>1 && lep2_match_idx<0) ){
          for(unsigned int genx = 0; genx < genps_p4().size() ; genx++){
            if(!genps_isLastCopy().at(genx) ) continue;
            if( abs(genps_id().at(genx))==abs(lep2.pdgid) ){
              double temp_dr = ROOT::Math::VectorUtil::DeltaR(genps_p4().at(genx), lep2.p4);
              if(temp_dr<matched_dr){
                lep2_match_idx=genx;
                gen_leps.FillCommon(genx);
                break;
              }
              else if(temp_dr<min_dr_lep2){
                min_dr_lep2=temp_dr;
                min_dr_lep2_idx=genx;
              }
            }
          }
          // if lep2 is still unmatched, fill with closest match, if possible
          if( lep2_match_idx<0 && min_dr_lep2_idx>0 ) gen_leps.FillCommon(min_dr_lep2_idx);
        }

      } // end if not data

      //////////////// calculate new met variable with the 2nd lepton removed
      float new_pfmet_x = 0.; //StopEvt.pfmet * std::cos(StopEvt.pfmet_phi);
      float new_pfmet_y = 0.;  //StopEvt.pfmet * std::sin(StopEvt.pfmet_phi);

      //
      // remove the second lepton, iso track, or tau from the MET
      //
      if (nVetoLeptons > 1) {
        new_pfmet_x += lep2.p4.px();
        new_pfmet_y += lep2.p4.py();
      }
      else if (!StopEvt.PassTrackVeto) {
        vecLorentzVector isotrk_p4s;
        for (unsigned int idx = 0; idx < Tracks.isoTracks_isVetoTrack_v3.size(); idx++) {
          if (!Tracks.isoTracks_isVetoTrack_v3.at(idx)) continue;
          isotrk_p4s.push_back(Tracks.isoTracks_p4.at(idx));
        }
        if (isotrk_p4s.size() == 0) {
          std::cout << "ERROR!!! Event fails iso track veto but no isolated track found!!!" << std::endl;
        }
        else {
          std::sort(isotrk_p4s.begin(), isotrk_p4s.end(), sortP4byPt());
          new_pfmet_x += isotrk_p4s.at(0).px();
          new_pfmet_y += isotrk_p4s.at(0).py();
        }
      }
      else if (!StopEvt.PassTauVeto) {
        vecLorentzVector tau_p4s;
        for (unsigned int idx = 0; idx < Taus.tau_isVetoTau_v2.size(); idx++) {
          if (!Taus.tau_isVetoTau_v2.at(idx)) continue;
          tau_p4s.push_back(Taus.tau_p4.at(idx));
        }
        if (tau_p4s.size() == 0) {
          std::cout << "ERROR!!! Event fails tau veto but no tau found!!!" << std::endl;
        }
        else {
          std::sort(tau_p4s.begin(), tau_p4s.end(), sortP4byPt());
          new_pfmet_x += tau_p4s.at(0).px();
          new_pfmet_y += tau_p4s.at(0).py();
        }
      }

      float new_pfmet_x_jup = new_pfmet_x + (StopEvt.pfmet_jup * std::cos(StopEvt.pfmet_phi_jup));
      float new_pfmet_y_jup = new_pfmet_y + (StopEvt.pfmet_jup * std::sin(StopEvt.pfmet_phi_jup));
      float new_pfmet_x_jdown = new_pfmet_x + (StopEvt.pfmet_jdown * std::cos(StopEvt.pfmet_phi_jdown));
      float new_pfmet_y_jdown = new_pfmet_y + (StopEvt.pfmet_jdown * std::sin(StopEvt.pfmet_phi_jdown));

      new_pfmet_x += StopEvt.pfmet * std::cos(StopEvt.pfmet_phi);
      new_pfmet_y += StopEvt.pfmet * std::sin(StopEvt.pfmet_phi);
      //
      // calclate new met quantities
      //
      StopEvt.pfmet_rl     = std::sqrt(new_pfmet_x*new_pfmet_x + new_pfmet_y*new_pfmet_y);
      StopEvt.pfmet_phi_rl = std::atan2(new_pfmet_y, new_pfmet_x);

      if (nVetoLeptons > 0) {
        StopEvt.mt_met_lep_rl = calculateMt(lep1.p4, StopEvt.pfmet_rl, StopEvt.pfmet_phi_rl);
        StopEvt.MT2W_rl = CalcMT2W_(mybjets,myaddjets,lep1.p4,StopEvt.pfmet_rl, StopEvt.pfmet_phi_rl);
        StopEvt.topnessMod_rl = CalcTopness_(1,StopEvt.pfmet_rl,StopEvt.pfmet_phi_rl,lep1.p4,mybjets,myaddjets);
      }

      if(jets.ngoodjets>1) StopEvt.mindphi_met_j1_j2_rl = getMinDphi(StopEvt.pfmet_phi_rl,jets.ak4pfjets_p4.at(0),jets.ak4pfjets_p4.at(1));

      //JEC up
      if(applyJECfromFile){
        StopEvt.pfmet_rl_jup     = std::sqrt(new_pfmet_x_jup*new_pfmet_x_jup + new_pfmet_y_jup*new_pfmet_y_jup);
        StopEvt.pfmet_phi_rl_jup = std::atan2(new_pfmet_y_jup, new_pfmet_x_jup);

        if (nVetoLeptons > 0) {
          StopEvt.mt_met_lep_rl_jup = calculateMt(lep1.p4, StopEvt.pfmet_rl_jup, StopEvt.pfmet_phi_rl_jup);
          StopEvt.MT2W_rl_jup = CalcMT2W_(mybjets_jup,myaddjets_jup,lep1.p4,StopEvt.pfmet_rl_jup, StopEvt.pfmet_phi_rl_jup);
          StopEvt.topnessMod_rl_jup = CalcTopness_(1,StopEvt.pfmet_rl_jup,StopEvt.pfmet_phi_rl_jup,lep1.p4,mybjets_jup,myaddjets_jup);
        }

        if(jets_jup.ngoodjets>1) StopEvt.mindphi_met_j1_j2_rl_jup = getMinDphi(StopEvt.pfmet_phi_rl_jup,jets_jup.ak4pfjets_p4.at(0),jets_jup.ak4pfjets_p4.at(1));

        //JEC down
        StopEvt.pfmet_rl_jdown     = std::sqrt(new_pfmet_x_jdown*new_pfmet_x_jdown + new_pfmet_y_jdown*new_pfmet_y_jdown);
        StopEvt.pfmet_phi_rl_jdown = std::atan2(new_pfmet_y_jdown, new_pfmet_x_jdown);

        if (nVetoLeptons > 0) {
          StopEvt.mt_met_lep_rl_jdown = calculateMt(lep1.p4, StopEvt.pfmet_rl_jdown, StopEvt.pfmet_phi_rl_jdown);
          StopEvt.MT2W_rl_jdown = CalcMT2W_(mybjets_jdown,myaddjets_jdown,lep1.p4,StopEvt.pfmet_rl_jdown, StopEvt.pfmet_phi_rl_jdown);
          StopEvt.topnessMod_rl_jdown = CalcTopness_(1,StopEvt.pfmet_rl_jdown,StopEvt.pfmet_phi_rl_jdown,lep1.p4,mybjets_jdown,myaddjets_jdown);
        }

        if(jets_jdown.ngoodjets>1) StopEvt.mindphi_met_j1_j2_rl_jdown = getMinDphi(StopEvt.pfmet_phi_rl_jdown,jets_jdown.ak4pfjets_p4.at(0),jets_jdown.ak4pfjets_p4.at(1));
      }
      //fill lepDPhi _rl
      if( nVetoLeptons > 0 ) {
        lep1.dphiMET_rl = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi_rl);
        if(applyJECfromFile){
          lep1.dphiMET_rl_jup = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi_rl_jup);
          lep1.dphiMET_rl_jdown = getdphi(lep1.p4.Phi(),StopEvt.pfmet_phi_rl_jdown);
        }
      }
      if( nVetoLeptons > 1 ) {
        lep2.dphiMET_rl = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi_rl);
        if(applyJECfromFile){
          lep2.dphiMET_rl_jup = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi_rl_jup);
          lep2.dphiMET_rl_jdown = getdphi(lep2.p4.Phi(),StopEvt.pfmet_phi_rl_jdown);
        }
      }

      bool pass_skim_met_photon = (fillPhoton)? (StopEvt.ph_met >= skim_met) : false;
      bool pass_skim_met_jup = (applyJECfromFile)? (StopEvt.pfmet_jup >= skim_met || StopEvt.pfmet_rl_jup >= skim_met) : false;
      bool pass_skim_met_jdown = (applyJECfromFile)? (StopEvt.pfmet_jdown >= skim_met || StopEvt.pfmet_rl_jdown >= skim_met) : false;

      if ( ((StopEvt.pfmet >= skim_met || StopEvt.pfmet_rl >= skim_met) || pass_skim_met_jup || pass_skim_met_jdown || pass_skim_met_photon) ) nEvents_pass_skim_met++;
      else if ( (jets.ngoodjets > 1) && (nLooseLeptons >= 2) && (lep1.pdgid*lep2.pdgid == -143) && (StopEvt.pfmet >= skim_met_emuEvt) ) nEvents_pass_skim_met_emuEvt++;
      else continue;
      //if(!(StopEvt.pfmet >= skim_met) && !(StopEvt.pfmet_rl >= skim_met) && !(StopEvt.pfmet_rl_jup >= skim_met) && !(StopEvt.pfmet_rl_jdown >= skim_met) && !(StopEvt.pfmet_jup >= skim_met) && !(StopEvt.pfmet_jdown >= skim_met) && !(StopEvt.pfmet_egclean >= skim_met) && !(StopEvt.pfmet_muegclean >= skim_met) && !(StopEvt.pfmet_muegcleanfix >= skim_met) && !(StopEvt.pfmet_uncorrcalomet >= skim_met) ) continue;
      /////////////////////////////////////////////////////////

      //
      // Trigger Information
      //
      //std::cout << "[babymaker::looper]: filling HLT vars" << std::endl;
      if (!isSignalFromFileName) {
        if (gconf.year == 2016) {
          // Triggers that were used in Moriond17 analysis (v24)
          StopEvt.HLT_SingleEl = ( passHLTTriggerPattern("HLT_Ele25_eta2p1_WPTight_Gsf_v") || passHLTTriggerPattern("HLT_Ele27_eta2p1_WPTight_Gsf_v") ||
                                   passHLTTriggerPattern("HLT_Ele27_WP85_Gsf_v") || passHLTTriggerPattern("HLT_Ele27_eta2p1_WPLoose_Gsf_v")
                                   );
          StopEvt.HLT_DiEl     = ( passHLTTriggerPattern("HLT_DoubleEle33_CaloIdL_MW_v") ||
                                   passHLTTriggerPattern("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v")
                                   );
          StopEvt.HLT_SingleMu = ( passHLTTriggerPattern("HLT_IsoMu20_v") || passHLTTriggerPattern("HLT_IsoTkMu20_v") ||
                                   passHLTTriggerPattern("HLT_IsoMu22_v") || passHLTTriggerPattern("HLT_IsoTkMu22_v") ||
                                   passHLTTriggerPattern("HLT_IsoMu24_v") || passHLTTriggerPattern("HLT_IsoTkMu24_v")
                                   );
          StopEvt.HLT_DiMu     = ( passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v") ||
                                   passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v")
                                   );
          StopEvt.HLT_MuE      = ( passHLTTriggerPattern("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v") || passHLTTriggerPattern("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                                   passHLTTriggerPattern("HLT_Mu8_TrkIsoVVL_Ele17_CaloIdL_TrackIdL_IsoVL_v") || passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v") ||
                                   passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v") || passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v")
                                   );
          StopEvt.HLT_MET_MHT  = ( passHLTTriggerPattern("HLT_PFMET100_PFMHT100_IDTight_v") || passHLTTriggerPattern("HLT_PFMETNoMu100_PFMHTNoMu100_IDTight_v") ||
                                   passHLTTriggerPattern("HLT_PFMET110_PFMHT110_IDTight_v") || passHLTTriggerPattern("HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_v") ||
                                   passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") || passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                   passHLTTriggerPattern("HLT_PFMETNoMu120_JetIdCleaned_PFMHTNoMu120_IDTight_v") || passHLTTriggerPattern("HLT_PFMETNoMu120_NoiseCleaned_PFMHTNoMu120_IDTight_v")
                                   );
          StopEvt.HLT_MET100_MHT100 = ( passHLTTriggerPattern("HLT_PFMET100_PFMHT100_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu100_PFMHTNoMu100_IDTight_v") ); // disabled most of the time
          StopEvt.HLT_MET110_MHT110 = ( passHLTTriggerPattern("HLT_PFMET110_PFMHT110_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_v") );
          StopEvt.HLT_MET120_MHT120 = ( passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") || passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_JetIdCleaned_PFMHTNoMu120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_NoiseCleaned_PFMHTNoMu120_IDTight_v")
                                        );
          StopEvt.HLT_MET = ( passHLTTriggerPattern("HLT_PFMET170_NoiseCleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMET170_JetIdCleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMET170_HBHECleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMET170_NotCleaned_v")
                              );
          // HT triggers for trigger efficiency - from MT2
          StopEvt.HLT_PFHT_unprescaled = passHLTTriggerPattern("HLT_PFHT800_v") || passHLTTriggerPattern("HLT_PFHT900_v");
          StopEvt.HLT_PFHT_prescaled = ( passHLTTriggerPattern("HLT_PFHT125_v") || passHLTTriggerPattern("HLT_PFHT200_v") ||
                                         passHLTTriggerPattern("HLT_PFHT300_v") || passHLTTriggerPattern("HLT_PFHT350_v") ||
                                         passHLTTriggerPattern("HLT_PFHT475_v") || passHLTTriggerPattern("HLT_PFHT600_v") );
          //as we use those only for trigger efficiency measurements, we actually don't care about the exact prescale ...
          StopEvt.HLT_AK8Jet_unprescaled = ( passHLTTriggerPattern("HLT_AK8PFJet360_TrimMass30_v") || passHLTTriggerPattern("HLT_AK8PFJet400_TrimMass30_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet500_v") || passHLTTriggerPattern("HLT_AK8PFJet450_v") );
          StopEvt.HLT_AK8Jet_prescaled   = ( passHLTTriggerPattern("HLT_AK8PFJet260_v") || passHLTTriggerPattern("HLT_AK8PFJet320_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet400_v") );
        }
        else if (gconf.year == 2017) {
          StopEvt.HLT_SingleEl = ( passHLTTriggerPattern("HLT_Ele35_WPTight_Gsf_v") // lowest unprescaled single electron trigger without caveats
                                || passHLTTriggerPattern("HLT_Ele32_WPTight_Gsf_v") // only present after run302026
                                || passHLTTriggerPattern("HLT_Ele32_WPTight_Gsf_L1DoubleEG_v") // super-set of above, in principle need to be reduced to Ele32
                                   );
          StopEvt.HLT_SingleMu = ( passHLTTriggerPattern("HLT_IsoMu27_v") ||
                                   passHLTTriggerPattern("HLT_IsoMu24_v") );
          StopEvt.HLT_DiEl = ( passHLTTriggerPattern("HLT_DoubleEle33_CaloIdL_MW_v") // loose ID, no isolation
                            || passHLTTriggerPattern("HLT_DoubleEle25_CaloIdL_MW_v") // from Run2017D onwards
                            || passHLTTriggerPattern("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL") // low pt, loose ID + isolation
                                 );
          StopEvt.HLT_DiMu = ( passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v") ||
                               passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v") );
          StopEvt.HLT_MuE  = ( passHLTTriggerPattern("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v") );
          StopEvt.HLT_MET_MHT = ( passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") ||
                                  passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                  passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60_v") ||
                                  passHLTTriggerPattern("HLT_PFMET100_PFMHT100_IDTight_PFHT60_v") ||
                                  passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_PFHT60_v") );
          StopEvt.HLT_MET110_MHT110 = ( passHLTTriggerPattern("HLT_PFMET110_PFMHT110_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_v") ); //prescaled
          StopEvt.HLT_MET120_MHT120 = ( passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMETTypeOne120_PFMHT120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMETTypeOne120_PFMHT120_IDTight_v") );
          StopEvt.HLT_MET130_MHT130 = ( passHLTTriggerPattern("HLT_PFMET130_PFMHT130_IDTight_v") );
          StopEvt.HLT_MET = ( passHLTTriggerPattern("HLT_PFMET250_HBHECleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMET300_HBHECleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMETTypeOne200_HBHE_BeamHaloCleaned_v") );
          // HT triggers
          StopEvt.HLT_PFHT_unprescaled = passHLTTriggerPattern("HLT_PFHT1050_v");
          StopEvt.HLT_PFHT_prescaled = ( passHLTTriggerPattern("HLT_PFHT430_v") || passHLTTriggerPattern("HLT_PFHT510_v") ||
                                         passHLTTriggerPattern("HLT_PFHT590_v") || passHLTTriggerPattern("HLT_PFHT680_v") ||
                                         passHLTTriggerPattern("HLT_PFHT780_v") || passHLTTriggerPattern("HLT_PFHT890_v") );
          StopEvt.HLT_AK8Jet_unprescaled = ( passHLTTriggerPattern("HLT_AK8PFJet360_TrimMass30_v") || passHLTTriggerPattern("HLT_AK8PFJet380_TrimMass30_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet400_TrimMass30_v") || passHLTTriggerPattern("HLT_AK8PFJet420_TrimMass30_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet500_v") || passHLTTriggerPattern("HLT_AK8PFJet550_v") );
          StopEvt.HLT_AK8Jet_prescaled   = ( passHLTTriggerPattern("HLT_AK8PFJet260_v") || passHLTTriggerPattern("HLT_AK8PFJet320_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet400_v") || passHLTTriggerPattern("HLT_AK8PFJet450_v") );
        }
        else if (gconf.year == 2018) {
          // TODO: confirm 2018 tirggers
          StopEvt.HLT_SingleEl = ( passHLTTriggerPattern("HLT_Ele35_WPTight_Gsf_v") ||
                                   passHLTTriggerPattern("HLT_Ele32_WPTight_Gsf_v") // same as 2017
                                   );
          StopEvt.HLT_SingleMu = ( passHLTTriggerPattern("HLT_IsoMu27_v") ||
                                   passHLTTriggerPattern("HLT_IsoMu24_v") );
          StopEvt.HLT_DiEl = ( passHLTTriggerPattern("HLT_DoubleEle33_CaloIdL_MW_v") // loose ID, no isolation
                            || passHLTTriggerPattern("HLT_DoubleEle25_CaloIdL_MW_v") // from Run2017D onwards
                            || passHLTTriggerPattern("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL") // low pt, loose ID + isolation
                                 );
          StopEvt.HLT_DiMu = ( passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v") ||
                               passHLTTriggerPattern("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v") );
          StopEvt.HLT_MuE  = ( passHLTTriggerPattern("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v") ||
                               passHLTTriggerPattern("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v") );
          StopEvt.HLT_MET_MHT = ( passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") ||
                                  passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                  passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60_v") ||
                                  passHLTTriggerPattern("HLT_PFMET100_PFMHT100_IDTight_PFHT60_v") ||
                                  passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_PFHT60_v") );
          StopEvt.HLT_MET110_MHT110 = ( passHLTTriggerPattern("HLT_PFMET110_PFMHT110_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_v") ); //prescaled
          StopEvt.HLT_MET120_MHT120 = ( passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMET120_PFMHT120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") ||
                                        passHLTTriggerPattern("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMETTypeOne120_PFMHT120_IDTight_HFCleaned_v") ||
                                        passHLTTriggerPattern("HLT_PFMETTypeOne120_PFMHT120_IDTight_v") );
          StopEvt.HLT_MET130_MHT130 = ( passHLTTriggerPattern("HLT_PFMET130_PFMHT130_IDTight_v") );
          StopEvt.HLT_MET = ( passHLTTriggerPattern("HLT_PFMET250_HBHECleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMET300_HBHECleaned_v") ||
                              passHLTTriggerPattern("HLT_PFMETTypeOne200_HBHE_BeamHaloCleaned_v") );

          // HT triggers -- similar to 2017
          StopEvt.HLT_PFHT_unprescaled = passHLTTriggerPattern("HLT_PFHT1050_v");
          StopEvt.HLT_PFHT_prescaled = ( passHLTTriggerPattern("HLT_PFHT430_v") || passHLTTriggerPattern("HLT_PFHT510_v") ||
                                         passHLTTriggerPattern("HLT_PFHT590_v") || passHLTTriggerPattern("HLT_PFHT680_v") ||
                                         passHLTTriggerPattern("HLT_PFHT780_v") || passHLTTriggerPattern("HLT_PFHT890_v") );
          // StopEvt.HLT_PFHT_MET_MHT = ( passHLTTriggerPattern("HLT_PFHT700_PFMET85_PFMHT85_IDTight_v") || );
          StopEvt.HLT_AK8Jet_unprescaled = ( passHLTTriggerPattern("HLT_AK8PFJet400_TrimMass30_v") || passHLTTriggerPattern("HLT_AK8PFJet420_TrimMass30_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet500_v") || passHLTTriggerPattern("HLT_AK8PFJet550_v") );
          StopEvt.HLT_AK8Jet_prescaled   = ( passHLTTriggerPattern("HLT_AK8PFJet260_v") || passHLTTriggerPattern("HLT_AK8PFJet320_v") ||
                                             passHLTTriggerPattern("HLT_AK8PFJet400_v") || passHLTTriggerPattern("HLT_AK8PFJet450_v") );
        }
        // Additional unprescaled jet trigger in JetHT
        StopEvt.HLT_CaloJet500_NoJetID = passHLTTriggerPattern("HLT_CaloJet500_NoJetID_v");
        // Photon triggers, store individual ones for all 3 years
        StopEvt.HLT_Photon50_R9Id90_HE10_IsoM  = passHLTTriggerPattern("HLT_Photon50_R9Id90_HE10_IsoM_v")  ? HLT_prescale(triggerName("HLT_Photon50_R9Id90_HE10_IsoM_v"))  : 0;
        StopEvt.HLT_Photon75_R9Id90_HE10_IsoM  = passHLTTriggerPattern("HLT_Photon75_R9Id90_HE10_IsoM_v")  ? HLT_prescale(triggerName("HLT_Photon75_R9Id90_HE10_IsoM_v"))  : 0;
        StopEvt.HLT_Photon90_R9Id90_HE10_IsoM  = passHLTTriggerPattern("HLT_Photon90_R9Id90_HE10_IsoM_v")  ? HLT_prescale(triggerName("HLT_Photon90_R9Id90_HE10_IsoM_v"))  : 0;
        StopEvt.HLT_Photon120_R9Id90_HE10_IsoM = passHLTTriggerPattern("HLT_Photon120_R9Id90_HE10_IsoM_v") ? HLT_prescale(triggerName("HLT_Photon120_R9Id90_HE10_IsoM_v")) : 0;
        StopEvt.HLT_Photon120                  = passHLTTriggerPattern("HLT_Photon120_v")                  ? HLT_prescale(triggerName("HLT_Photon120_v"))                  : 0;
        // unprescaled triggers for 2016 but prescaled in 2017 & 2018
        StopEvt.HLT_Photon165_R9Id90_HE10_IsoM = passHLTTriggerPattern("HLT_Photon165_R9Id90_HE10_IsoM_v") ? HLT_prescale(triggerName("HLT_Photon165_R9Id90_HE10_IsoM_v")) : 0;
        StopEvt.HLT_Photon165_HE10             = passHLTTriggerPattern("HLT_Photon165_HE10_v")             ? HLT_prescale(triggerName("HLT_Photon165_HE10_v"))             : 0;
        StopEvt.HLT_Photon175                  = passHLTTriggerPattern("HLT_Photon175_v")                  ? HLT_prescale(triggerName("HLT_Photon175_v"))                  : 0;
        // unprescaled triggers in SinglePhoton
        StopEvt.HLT_Photon200                  = passHLTTriggerPattern("HLT_Photon200_v");      // in 2017 & 2018 only
        StopEvt.HLT_Photon250_NoHE             = passHLTTriggerPattern("HLT_Photon250_NoHE_v"); // in 2016 only
        StopEvt.HLT_Photon300_NoHE             = passHLTTriggerPattern("HLT_Photon300_NoHE_v");
      }

      ///////////////////////////////////////////////////////////

      //
      // Fill Tree
      //
      BabyTree->Fill();

    }//close event loop
    //
    // Close input file
    //
    file->Close();
    delete file;
  }//close file loop

  //
  // Write and Close baby file
  //
  BabyFile->cd();
  // save counter histogram
  BabyTree->Write();
  counterhist->Write();
  if(isSignalFromFileName){
    counterhistSig->Write();
    histNEvts->Write();
  }
  BabyFile->Close();
  //delete BabyFile;
  //histFile->cd();

  if (runTopCandTreeMaker) {
    topcandTreeMaker->Write();
  }

  //
  // Some clean up
  //
  if(isSignalFromFileName) {
    fxsec->Close();
    delete fxsec;
  }
  if(!isDataFromFileName){
    pileupfile->Close();
    delete pileupfile;
  }
  if (applyBtagSFs) {
    jets.deleteBtagSFTool();
  }

  //
  // Benchmarking
  //
  bmark->Stop("benchmark");

  //
  // Print Skim Cutflow
  //
  cout << endl;
  cout << "Wrote babies into file " << BabyFile->GetName() << endl;
  // cout << "Wrote hists into file " << histFile->GetName() << endl;
  cout << "-----------------------------" << endl;
  cout << "Events Processed                     " << nEvents_processed << endl;
  cout << "Events with " << skim_nvtx << " Good Vertex            " << nEvents_pass_skim_nVtx << endl;
  cout << "Events with at least " << skim_nJets    << " Good Jets     " << nEvents_pass_skim_nJets << endl;
  cout << "Events with at least " << skim_nBJets   << " Good BJets    " << nEvents_pass_skim_nBJets << endl;
  cout << "Events with at least " << skim_nGoodLep << " Good Lepton   " << nEvents_pass_skim_nGoodLep << endl;
  cout << "Events with apply2ndLepVeto=" << boolalpha << apply2ndLepVeto << "    " << nEvents_pass_skim_2ndlepVeto << endl;
  cout << "Events with MET > " << skim_met << " GeV            " << nEvents_pass_skim_met << endl;
  cout << "Extra emu events with MET > " << skim_met_emuEvt << " GeV   " << nEvents_pass_skim_met_emuEvt << endl;
  cout << "-----------------------------" << endl;
  cout << "CPU  Time:   " << Form( "%.01f", bmark->GetCpuTime("benchmark")  ) << endl;
  cout << "Real Time:   " << Form( "%.01f", bmark->GetRealTime("benchmark") ) << endl;
  cout << endl;
  delete bmark;

  return 0;

}
