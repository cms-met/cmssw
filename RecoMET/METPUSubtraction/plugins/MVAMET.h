#ifndef MVAMET_h
#define MVAMET_h

/** \class MVAMET
 * */

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include <iostream>
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/METReco/interface/PFMETCollection.h"
#include <DataFormats/METReco/interface/PFMET.h>
#include <DataFormats/PatCandidates/interface/MET.h>
#include <DataFormats/PatCandidates/interface/Tau.h>
#include <DataFormats/PatCandidates/interface/Muon.h>
#include <DataFormats/PatCandidates/interface/Jet.h>
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "CondFormats/EgammaObjects/interface/GBRForest.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"

#include <TFile.h>
#include <TVector2.h>
#include <TMath.h>

// class storing the MET and recoil information
class metPlus : public pat::MET {
public:
  int METFlag;
  std::string collection_name;
  metPlus() {}
  metPlus(pat::MET mother) : pat::MET(mother), 
      METFlag(-1),
      collection_name("unset") {}
  //function to return METFlags that tell whether the MET was made from PV charged and neutral components
  bool containsCharged() { return ((this->METFlag == 0) || (this->METFlag == 1)); }
  bool containsNeutral() { return ((this->METFlag == 0) || (this->METFlag == 2)); } 
};


// class to store constituents of the recoil
// because the tau is a composite object this information can be stored and accessed with this class
class recoilComponent {
 private:
  edm::Ptr<reco::Candidate> srcLepton_;
  reco::Candidate::LorentzVector p4_;
 public:
  std::vector<reco::CandidatePtr> chargedTauJetCandidates;
  std::vector<reco::CandidatePtr> neutralTauJetCandidates;
  recoilComponent(edm::Ptr<reco::Candidate> srcLepton) : srcLepton_(srcLepton) {}
  edm::Ptr<reco::Candidate> getSrcLepton() const { return srcLepton_; }
  reco::Candidate::LorentzVector p4() const { return this->p4_; }
  void setP4(const reco::Candidate::LorentzVector & p4) { p4_ = p4; }
  reco::Candidate::LorentzVector chargedP4() const
  {
    reco::Candidate::LorentzVector p4;
    for(const auto & tauJet: chargedTauJetCandidates)
      p4 += tauJet->p4();
    return p4+this->p4();
  }

  reco::Candidate::LorentzVector neutralP4() const
  {
    reco::Candidate::LorentzVector p4;
    for(const auto & tauJet: neutralTauJetCandidates)
      p4 += tauJet->p4();
    return p4;
  }

  float chargedSumEt() const
  {
    float sumEt = 0;
    for(const auto & tauJet: chargedTauJetCandidates)
      sumEt += tauJet->p4().pt();
    return sumEt + this->p4().pt();
  }

  float neutralSumEt() const
  {
    float sumEt = 0;
    for(const auto & tauJet: neutralTauJetCandidates)
      sumEt += tauJet->p4().pt();
    return sumEt;
  }
  int pdgId() const { return this->srcLepton_->pdgId(); }
  bool isMuon() const { return (abs(pdgId()) == 13); }
};

class recoilingBoson : public reco::Particle {
  // tag: taggs the combination with the highest pt
  // might be dropped since 3rd lepton veto is applied anyway for the training
  bool tag;

 public: 
  std::vector<recoilComponent> leptons;
  recoilingBoson() : tag(false) {}
  void addLepton(recoilComponent rComp) { this->leptons.push_back(rComp); }
  bool isDiMuon() const { return (leptons.size() == 2) ? (leptons[0].isMuon() and leptons[1].isMuon()) : false; }
  bool select() const { return this->p4vec().M() > 80 && this->p4vec().M() < 100 && this->isDiMuon(); }
  void setTagged() { this->tag = true; } 
  bool isTagged() const { return this->tag; } 

  reco::Candidate::LorentzVector p4vec() const { return (this->chargedP4() + this->neutralP4()); }

  reco::Candidate::LorentzVector chargedP4() const
  {
    reco::Candidate::LorentzVector p4;
    for(const auto & lepton: leptons)
      p4 += lepton.chargedP4();

    return p4;
  }

  reco::Candidate::LorentzVector neutralP4() const
  {
    reco::Candidate::LorentzVector p4;
    for(const auto & lepton: leptons)
      p4 += lepton.neutralP4();

    return p4;
  }

  float chargedSumEt() const
  {
    float sumEt = 0;
    for(const auto & lepton: leptons)
      sumEt += lepton.chargedSumEt();

    return sumEt; 
  }

  float neutralSumEt() const
  {
    float sumEt = 0;
    for(const auto & lepton: leptons)
      sumEt += lepton.neutralSumEt();

    return sumEt; 
  }

  double sumEt() const { return (chargedSumEt() + neutralSumEt()); }
  int getPdgId(int index) const { return leptons[index].pdgId(); }
};

class MVAMET : public edm::stream::EDProducer<> {

 public:

  // basic constructor from parameter set
  MVAMET(const edm::ParameterSet&);
  ~MVAMET();
  
  void produce(edm::Event&, const edm::EventSetup&);
  typedef std::vector<edm::InputTag> vInputTag;
  float bestMass_;
  // create a vector given input variables
  Float_t* createFloatVector(std::vector<std::string> variableNames);

  unsigned int countJets(const pat::JetCollection& jets, const float maxPt);

  // load MVA file produced in the training
  const GBRForest* loadMVAfromFile(const edm::FileInPath& inputFileName, std::vector<std::string>& trainingVariableNames, std::string mvaName);
  // read the response
  const Float_t GetResponse(const GBRForest * Reader,std::vector<std::string> &variableNames );

  // to correctly create the map of regression input vriables
  void addToMap(const reco::Candidate::LorentzVector p4, const double sumEt, const std::string &type);
  void addToMap(const metPlus &recoil, const metPlus &referenceMET);
  void addToMap(const recoilingBoson &Z);
  void addToMap(const metPlus &recoil, const recoilingBoson &Z);

  metPlus calculateRecoil(metPlus* MET, const recoilingBoson &Z, edm::Event& evt);
  void TagZ();
private:
  void doCombinations(int offset, int k);
  void unpackCompositeCands(edm::Event& evt);
  void saveMap(edm::Event& evt);
  void calculateRecoilingObjects(edm::Event& evt, const pat::MuonCollection&, const pat::TauCollection& );
  void cleanLeptonsFromSS()
  { 
    combinations_.erase(std::remove_if(combinations_.begin(), combinations_.end(), [](std::vector<edm::Ptr<reco::Candidate>> pair) { return pair[0]->charge() == pair[1]->charge(); }), combinations_.end());
   }
  void handleMuons(edm::Ptr<reco::Candidate> lepton, recoilingBoson& Z, const pat::MuonCollection& );
  void handleTaus(edm::Ptr<reco::Candidate> lepton, recoilingBoson& Z, const pat::TauCollection& );
  void fillEventInformation(edm::Event&);
  std::string mvaMETLabel_;

  vInputTag srcMETTags_;
  
  std::vector<edm::EDGetTokenT<pat::METCollection > >  srcMETs_;
  edm::EDGetTokenT<reco::VertexCollection>             srcVertices_;
  edm::EDGetTokenT<pat::JetCollection>                 srcJets_;
  std::vector<edm::EDGetTokenT<reco::CandidateView > > srcLeptons_;
  edm::EDGetTokenT<pat::TauCollection>                 srcTaus_;
  edm::EDGetTokenT<pat::MuonCollection>                srcMuons_;
  edm::EDGetTokenT<math::Error<2>::type> srcTausSignificance_; 
  bool useTauSig_;
  
  std::vector<int> srcMETFlags_;
  
  std::map<std::string, Float_t> var_;
  
  std::vector<edm::Ptr<reco::Candidate>> allLeptons_;
  std::vector<std::vector<edm::Ptr<reco::Candidate>>> combinations_;
  std::vector<edm::Ptr<reco::Candidate>> combination_;
  
  std::vector<std::string> variablesForPhiTraining_  = {};
  std::vector<std::string> variablesForRecoilTraining_  = {};
  std::vector<std::string> variablesForCovU1_  = {};
  std::vector<std::string> variablesForCovU2_  = {};

  const GBRForest* mvaReaderPhiCorrection_;
  const GBRForest* mvaReaderRecoilCorrection_;
  const GBRForest* mvaReaderCovU1_;
  const GBRForest* mvaReaderCovU2_;

  bool debug_;
  bool saveMap_;
  bool produceRecoils_;
  std::vector<recoilingBoson> Bosons_;
  size_t combineNLeptons_;
  bool requireOS_;
  edm::Handle<pat::METCollection> referenceMETHandle_;
  bool permuteLeptonsWithinPlugin_;
  edm::EDGetTokenT<reco::CompositeCandidateCollection> leptonPermutationsHandle_;
  // to be removed
  const reco::GenMET * genMET_;
}; 
#endif
