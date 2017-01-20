#include "FWCore/Framework/interface/stream/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Common/interface/View.h"

#include "DataFormats/Common/interface/Association.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "CommonTools/Utils/interface/StringCutObjectSelector.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"

#include "DataFormats/PatCandidates/interface/Muon.h"



class BadGlobalMuonTaggerPAT : public edm::stream::EDFilter<> {
public:
  explicit BadGlobalMuonTaggerPAT(const edm::ParameterSet & iConfig);
  virtual ~BadGlobalMuonTaggerPAT() {}

  virtual bool filter(edm::Event & iEvent, const edm::EventSetup & iSetup) override;

private:
  edm::EDGetTokenT<edm::View<pat::Muon>> muons_;            
  edm::EDGetTokenT<std::vector<reco::Vertex>> vtx_;            
  double ptCut_;
  bool   selectClones_, verbose_;

  bool outInOnly(const pat::Muon &mu) const {
    const reco::Track &tk = *mu.innerTrack();
    return tk.algoMask().count() == 1 && tk.isAlgoInMask(reco::Track::muonSeededStepOutIn);
  }
  bool preselection(const pat::Muon &mu) const { 
    return mu.isGlobalMuon() && (!selectClones_ || outInOnly(mu));
  }
  bool tighterId(const pat::Muon &mu) const { 
    return muon::isMediumMuon(mu) && mu.numberOfMatchedStations() >= 2; 
  }
  bool tightGlobal(const pat::Muon &mu) const {
    return (mu.globalTrack()->hitPattern().muonStationsWithValidHits() >= 3 && mu.globalTrack()->normalizedChi2() <= 20);
  }
  bool safeId(const pat::Muon &mu) const { 
    if (mu.muonBestTrack()->ptError() > 0.2 * mu.muonBestTrack()->pt()) { return false; }
    return mu.numberOfMatchedStations() >= 1 || tightGlobal(mu);
  }
  bool partnerId(const pat::Muon &mu) const {
    return mu.pt() >= 10 && mu.numberOfMatchedStations() >= 1;
  }
};

BadGlobalMuonTaggerPAT::BadGlobalMuonTaggerPAT(const edm::ParameterSet & iConfig) :
  muons_(consumes<edm::View<pat::Muon>>(iConfig.getParameter<edm::InputTag>("muons"))),
  vtx_(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("vtx"))),
  ptCut_(iConfig.getParameter<double>("muonPtCut")),
  selectClones_(iConfig.getParameter<bool>("selectClones"))
{

  produces<std::vector<pat::Muon> >();
}


bool 
BadGlobalMuonTaggerPAT::filter(edm::Event & iEvent, const edm::EventSetup & iSetup) {
  using namespace edm;

  // read input
  Handle<edm::View<pat::Muon>> hmuons;
  Handle<std::vector<reco::Vertex>> vtx;
  std::vector<int> goodMuon;

  iEvent.getByToken(vtx_,  vtx);
  assert(vtx->size() >= 1);
  const auto &PV = vtx->front().position();
 
  iEvent.getByToken(muons_,  hmuons);
  const edm::View<pat::Muon> & muons = *hmuons;

  auto outputMuons = std::make_unique<std::vector<pat::Muon> >();

  for (const pat::Muon & mu : muons) {
    if (!mu.isPFMuon() || mu.innerTrack().isNull()) {
      goodMuon.push_back(-1); // bad but we don't care
      continue;
    } 
    if (preselection(mu)) {
      float dxypv = std::abs(mu.innerTrack()->dxy(PV));
      float dzpv  = std::abs(mu.innerTrack()->dz(PV));
      if (tighterId(mu)) {
	bool ipLoose = ((dxypv < 0.5 && dzpv < 2.0) || mu.innerTrack()->hitPattern().pixelLayersWithMeasurement() >= 2);
	goodMuon.push_back(ipLoose || (!selectClones_ && tightGlobal(mu)));
      } else if (safeId(mu)) {
	bool ipTight = (dxypv < 0.2 && dzpv < 0.5);
	goodMuon.push_back(ipTight);
      } else {
	goodMuon.push_back(0);
      }
    } else {
      goodMuon.push_back(3); // maybe good, maybe bad, but we don't care
    }
  }

  for (unsigned int i = 0, n = muons.size(); i < n; ++i) {
    if (muons[i].pt() < ptCut_ || goodMuon[i] != 0) {
      continue;
    }
    if (verbose_) printf("potentially bad muon %d of pt %.1f eta %+.3f phi %+.3f\n", int(i+1), muons[i].pt(), muons[i].eta(), muons[i].phi());
    bool bad = true;
    if (selectClones_) {
      bad = false; // unless proven otherwise
      unsigned int n1 = muons[i].numberOfMatches(pat::Muon::SegmentArbitration);
      for (unsigned int j = 0; j < n; ++j) {
	if (j == i || goodMuon[j] <= 0 || !partnerId(muons[j])) continue;
	unsigned int n2 = muons[i].numberOfMatches(pat::Muon::SegmentArbitration);
	if (deltaR2(muons[i],muons[j]) < 0.16 || (n1 > 0 && n2 > 0 && muon::sharedSegments(muons[i],muons[j]) >= 0.5*std::min(n1,n2))) {
	  if (verbose_) printf("     tagged as clone of muon %d of pt %.1f eta %+.3f phi %+.3f\n", int(j+1), muons[j].pt(), muons[j].eta(), muons[j].phi());
	  bad = true;
	  break;
	} 
      }
    }
    if(bad) { //found = true;
      outputMuons->push_back(muons[i] );
    }
  }

  iEvent.put(std::move(outputMuons));
  
  //if(filter_) {
  //return found;
  //}
  return true;
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(BadGlobalMuonTaggerPAT);
