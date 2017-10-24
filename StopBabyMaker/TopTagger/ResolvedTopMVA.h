/*
 * ResolvedTopMVA.h
 *
 *  Created on: Oct 6, 2016
 *      Author: hqu
 *  Modified on: Oct 23, 2017
 */

#ifndef STOPBABYMAKER_TOPTAGGER_RESOLVEDTOPMVA_H
#define STOPBABYMAKER_TOPTAGGER_RESOLVEDTOPMVA_H

// #include "AnalysisTools/DataFormats/interface/Jet.h"
// #include "AnalysisTools/Utilities/interface/PhysicsUtilities.h"
#include "CMS3.h"
#include "TMVAReader.h"
// #include "AnalysisTools/Utilities/interface/PartonMatching.h"

class TopCand {
 public:
  TopCand() {}
  TopCand(const size_t ib, const size_t ij2, const size_t ij3) : disc(-1), ib_(ib), ij2_(ij2), ij3_(ij3) {
    wcand = cms3.pfjets_p4().at(ij2) + cms3.pfjets_p4().at(ij3);
    topcand = cms3.pfjets_p4().at(ib) + wcand;
  }

  bool passMassW(double range=40)   const { return std::abs(wcand.mass()-80)    <= range; }
  bool passMassTop(double range=80) const { return std::abs(topcand.mass()-175) <= range; }
  bool sameAs(const TopCand &c) const { return ib_==c.ib_ && ij2_==c.ij2_ && ij3_==c.ij3_; }
  bool overlaps(const TopCand &c) const {
    return (ib_==c.ib_ || ib_==c.ij2_ || ib_==c.ij3_ || ij2_==c.ib_ || ij2_==c.ij2_ ||
            ij2_==c.ij3_ || ij3_==c.ib_ || ij3_==c.ij2_ || ij3_==c.ij3_);
  }

  std::map<TString,float> calcTopCandVars();

  LorentzVector wcand;
  LorentzVector topcand;

  double disc;

  friend std::ostream &operator<<(std::ostream &os, const TopCand &c);

 private:
  size_t ib_;
  size_t ij2_;
  size_t ij3_;

};

class ResolvedTopMVA {
 public:
  ResolvedTopMVA(TString weightfile, TString mvaname);
  virtual ~ResolvedTopMVA();

  static constexpr double WP_ALL    = -1.0; // used for candidate studies
  static constexpr double WP_LOOSE  = 0.83;
  static constexpr double WP_MEDIUM = 0.98;
  static constexpr double WP_TIGHT  = 0.99;
  std::vector<TopCand> getTopCandidates(const double WP=WP_TIGHT);

 private:
  void initTopMVA();
  std::vector<TopCand> removeOverlap(std::vector<TopCand> &cands, double threshold);

  TMVAReader mvaReader;
  std::vector<TString> vars;

};

#endif /* STOPBABYMAKER_TOPTAGGER_RESOLVEDTOPMVA_H */
