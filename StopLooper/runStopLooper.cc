#include <iostream>
#include <string>
#include "TChain.h"
#include "TString.h"

#include "StopLooper.h"

using namespace std;

int main(int argc, char** argv)
{

  if (argc < 4) {
    cout << "USAGE: runStopLooper <input_dir> <sample> <output_dir>" << endl;
    return 1;
  }

  string input_dir(argv[1]);
  string sample(argv[2]);
  string output_dir(argv[3]);

  TChain *ch = new TChain("t");
  TString infile = Form("%s/%s*.root", input_dir.c_str(), sample.c_str());
 
  // 2017Data
  bool doData2017B = false;
  bool doData2017C = false;
  bool doData2017dilep = false;
  bool doData2016 = false;
  bool doData2016dilep = false;
  bool doData2016B = false;
  bool doData2016C = false;
  bool doData2016D = false;
  bool doData2016E = false;
  bool doData2016F = false;
  bool doData2016G = false;
  bool doData2016H = false;
  bool doData2016Sync = false;

  if (sample == "data_2017all") {
    doData2017B = true;
    doData2017C = true;
  }
  // else if (sample == "data_2017B") doData2017B = true;
  // else if (sample == "data_2017C") doData2017C = true;
  else if (sample == "data_2017dilep") doData2017dilep = true;
  else if (sample == "data_2016") doData2016 = true;
  else if (sample == "data_2016B") doData2016B = true;
  else if (sample == "data_2016C") doData2016C = true;
  else if (sample == "data_2016D") doData2016D = true;
  else if (sample == "data_2016E") doData2016E = true;
  else if (sample == "data_2016F") doData2016F = true;
  else if (sample == "data_2016G") doData2016G = true;
  else if (sample == "data_2016H") doData2016H = true;
  else if (sample == "data_2016dilep") doData2016dilep = true;
  else if (sample == "data_2016Sync") doData2016Sync = true;
  else if (sample == "data_2016all") {
    doData2016 = true;
    doData2016dilep = true;
  } else {
    cout << __FILE__ << ':' << __LINE__ << ": infile= " << infile << endl;
    ch->Add(infile);
  }

  if (doData2017B) {
    ch->Add(Form("%s/data_met_run2017B_v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_run2017B_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singleel_run2017B_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singleel_run2017B_v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singlemu_run2017B_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singlemu_run2017B_v2.root", input_dir.c_str()));
  }
  if (doData2017C) {
    ch->Add(Form("%s/data_singlemu_run2017C_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singlemu_run2017C_v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singlemu_run2017C_v3.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singleel_run2017C_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singleel_run2017C_v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_singleel_run2017C_v3.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_run2017C_v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_run2017C_v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_run2017C_v3.root", input_dir.c_str()));
  }
  if (doData2017dilep) {
    ch->Add(Form("%s/data_double_eg_Run2017B_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2017B_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2017C_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2017C_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2017C_MINIAOD_PromptReco-v3.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2017B_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2017B_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2017B_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2017B_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2017C_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2017C_MINIAOD_PromptReco-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2017C_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2017C_MINIAOD_PromptReco-v3.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2017C_MINIAOD_PromptReco-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2017C_MINIAOD_PromptReco-v3.root", input_dir.c_str()));
  }
  
  if (doData2016Sync) {
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016B-03Feb2017_ver2-v2/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016C-03Feb2017-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016D-03Feb2017-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016E-03Feb2017-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016F-03Feb2017-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016G-03Feb2017-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016H-03Feb2017_ver2-v1/skim/*.root");
    ch->Add("/hadoop/cms/store/user/haweber/AutoTwopler_babies/Stop_1l_v24/MET_Run2016H-03Feb2017_ver3-v1/skim/*.root");
    ch->Add("/nfs-7/userdata//haweber/tupler_babies/merged/Stop_1l/v24_trulyunmerged/skim/data_single_electron_*.root");
    ch->Add("/nfs-7/userdata//haweber/tupler_babies/merged/Stop_1l/v24_trulyunmerged/skim/data_single_muon_*.root");
  }

  if (doData2016) {
    doData2016B = true;
    doData2016C = true;
    doData2016D = true;
    doData2016E = true;
    doData2016F = true;
    doData2016G = true;
    doData2016H = true;
  }

  if (doData2016B) {
    ch->Add(Form("%s/data_met_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
  }
  if (doData2016C) {
    ch->Add(Form("%s/data_single_muon_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
  }
  if (doData2016D) {
    ch->Add(Form("%s/data_single_muon_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
  }
  if (doData2016E) {
    ch->Add(Form("%s/data_met_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
  }
  if (doData2016F) {
    ch->Add(Form("%s/data_met_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
  }
  if (doData2016G) {
    ch->Add(Form("%s/data_met_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
  }
  if (doData2016H) {
    ch->Add(Form("%s/data_single_electron_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_electron_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_single_muon_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_met_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
  }

  if (doData2016dilep) {
    ch->Add(Form("%s/data_double_eg_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_eg_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_double_mu_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016B_MINIAOD_03Feb2017_ver2-v2.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016C_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016E_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016F_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016G_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016H_MINIAOD_03Feb2017_ver3-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016D_MINIAOD_03Feb2017-v1.root", input_dir.c_str()));
    ch->Add(Form("%s/data_muon_eg_Run2016H_MINIAOD_03Feb2017_ver2-v1.root", input_dir.c_str()));
  }

  if (ch->GetEntries() == 0) {
    cout << "ERROR: no entries in chain. filename was: " << infile << endl;
    return 2;
  }

  StopLooper stop;
  stop.looper(ch, sample, output_dir);

  return 0;
}
