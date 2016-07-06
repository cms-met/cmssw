#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
 
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
 
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace edm;
using namespace pat;


////////////////////////////////////////////////////////////////////////////////
// class definition
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
class patMuonIDIsoSelector : public edm::stream::EDProducer<> {

public:
  // construction/destruction
  patMuonIDIsoSelector(const edm::ParameterSet& iConfig);
  ~patMuonIDIsoSelector() {;}
  
  // member functions
  void produce(edm::Event& iEvent,const edm::EventSetup& iSetup);
  void endJob();

private:
  // input muon collection to be considered
  edm::InputTag src_;
  // rho collection in case of effective area
  edm::InputTag rho_;
  // vertex collection
  edm::InputTag vertex_;

  // value map with re-calcuated isolations from iso-deposits .. different wrt to the ones already inside the muon object
  edm::InputTag charged_hadron_iso_;
  edm::InputTag neutral_hadron_iso_;
  edm::InputTag photon_iso_;

  // isolation, pt and eta cut to be applied
  double relativeIsolationCut_;
  double ptCut_;
  double etaCut_;
  double ptThresholdForHighPt_;

  // tipe of muon id : tightID, mediumID, looseID, softID and highPt
  std::string  typeID_;

  // type of relative isolation cut
  std::string   typeIso_;

  // tokens
  edm::EDGetTokenT<pat::MuonCollection>    srcToken_ ;
  edm::EDGetTokenT<double>                 rhoToken_ ;
  edm::EDGetTokenT<reco::VertexCollection> vertexToken_ ;

  edm::EDGetTokenT<edm::ValueMap<double> > charged_hadron_isoToken_ ;
  edm::EDGetTokenT<edm::ValueMap<double> > neutral_hadron_isoToken_ ;
  edm::EDGetTokenT<edm::ValueMap<double> > photon_isoToken_ ;

};




////////////////////////////////////////////////////////////////////////////////
// construction/destruction
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
patMuonIDIsoSelector::patMuonIDIsoSelector(const edm::ParameterSet& iConfig){

  
  if(iConfig.existsAs<edm::InputTag >("src")){
    src_ = iConfig.getParameter<edm::InputTag>("src");
    if(src_ == edm::InputTag(""))
      throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] empty muon tag is given --> at least one \n"; 
  }
  else throw cms::Exception("Configuration")<<"[SelectedElectronFEDListProducer] no muon tag is given \n";  

  if(iConfig.existsAs<edm::InputTag >("vertex")){
    vertex_ = iConfig.getParameter<edm::InputTag>("vertex");
    if(vertex_ == edm::InputTag(""))
      throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] empty vertex tag is given --> at least one \n"; 
  }
  else throw cms::Exception("Configuration")<<"[SelectedElectronFEDListProducer] no vertex tag is given \n";  

  if(iConfig.existsAs<edm::InputTag >("rho"))
    rho_ = iConfig.getParameter<edm::InputTag>("rho");
  

  if(iConfig.existsAs<edm::InputTag >("charged_hadron_iso"))
    charged_hadron_iso_ = iConfig.getParameter<edm::InputTag>("charged_hadron_iso");
  

  if(iConfig.existsAs<edm::InputTag >("neutral_hadron_iso"))
    neutral_hadron_iso_ = iConfig.getParameter<edm::InputTag>("neutral_hadron_iso");
  

  if(iConfig.existsAs<edm::InputTag >("photon_iso"))
    photon_iso_ = iConfig.getParameter<edm::InputTag>("photon_iso");
  

  if(iConfig.existsAs<double>("relativeIsolationCut"))
    relativeIsolationCut_ = iConfig.getParameter<double>("relativeIsolationCut");
  else
    relativeIsolationCut_ = 0.2;

  if(iConfig.existsAs<double>("ptCut"))
    ptCut_ = iConfig.getParameter<double>("ptCut");
  else
    ptCut_ = 10.;

  if(iConfig.existsAs<double>("etaCut"))
    etaCut_ = iConfig.getParameter<double>("etaCut");
  else
    etaCut_ = 2.4;
  
  if(iConfig.existsAs<std::string>("typeID"))
    typeID_ = iConfig.getParameter<std::string>("typeID");
  else
    typeID_ = "TightID";

  if(iConfig.existsAs<std::string>("typeIso"))
    typeIso_ = iConfig.getParameter<std::string>("typeIso");
  else
    typeIso_ = "dBeta";

  if(iConfig.existsAs<double>("ptThresholdForHighPt"))
     ptThresholdForHighPt_ = iConfig.getParameter<double>("ptThresholdForHighPt");
  else
    ptThresholdForHighPt_ = 100;

  // tokens
  if(!(src_ == edm::InputTag(""))) 
    srcToken_ = consumes<pat::MuonCollection>(src_);

  if(!(rho_ == edm::InputTag(""))) 
    rhoToken_ = consumes<double>(rho_);

  if(!(vertex_ == edm::InputTag(""))) 
    vertexToken_ = consumes<reco::VertexCollection>(vertex_);

  if(!(charged_hadron_iso_ == edm::InputTag(""))) 
    charged_hadron_isoToken_ = consumes<edm::ValueMap<double> >(charged_hadron_iso_);

  if(!(neutral_hadron_iso_ == edm::InputTag(""))) 
    neutral_hadron_isoToken_ = consumes<edm::ValueMap<double> >(neutral_hadron_iso_);

  if(!(photon_iso_ == edm::InputTag(""))) 
    photon_isoToken_ = consumes<edm::ValueMap<double> >(photon_iso_);

   produces<pat::MuonCollection>();
}


////////////////////////////////////////////////////////////////////////////////
// implementation of member functions
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
void patMuonIDIsoSelector::produce(edm::Event& iEvent,const edm::EventSetup& iSetup){

  edm::Handle<pat::MuonCollection> MuonCollectionHandle;
  if(!(src_ == edm::InputTag(""))){  
    iEvent.getByToken(srcToken_,MuonCollectionHandle);
    if(MuonCollectionHandle.failedToGet()){
      throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] failed to get muon collection \n"; 
    }
  }    

  edm::Handle<reco::VertexCollection> VertexCollectionHandle;
  if(!(vertex_ == edm::InputTag(""))) { 
    iEvent.getByToken(vertexToken_,VertexCollectionHandle);
    if(VertexCollectionHandle.failedToGet()) {
      throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] failed to get vertex collection \n"; 
    }
  }

  const reco::Vertex& primaryVertex = VertexCollectionHandle->at(0);

  edm::Handle<double> rhoHandle;
  double rho = -1;
  if(!(rho_ == edm::InputTag(""))) { 
    iEvent.getByToken(rhoToken_,rhoHandle);
    if(!rhoHandle.failedToGet()){
      rho = *rhoHandle;
    }
  }

  // take the isolation maps 
  bool isGoodChargeIsoMap = false;
  edm::Handle<edm::ValueMap<double>> chargeHadronIsoHandle;
  if(!(charged_hadron_iso_ == edm::InputTag(""))){
    iEvent.getByToken(charged_hadron_isoToken_,chargeHadronIsoHandle);
    if(!chargeHadronIsoHandle.failedToGet())
      isGoodChargeIsoMap = true;
  }

  bool isGoodNeutralIsoMap = false;
  edm::Handle<edm::ValueMap<double>> neutralHadronIsoHandle;
  if(!(neutral_hadron_iso_ == edm::InputTag(""))){
    iEvent.getByToken(neutral_hadron_isoToken_,neutralHadronIsoHandle);
    if(!neutralHadronIsoHandle.failedToGet())
      isGoodNeutralIsoMap = true;
  }

  bool isGoodPhotonIsoMap = false;
  edm::Handle<edm::ValueMap<double>> photonIsoHandle;
  if(!(photon_iso_ == edm::InputTag(""))){
    iEvent.getByToken(photon_isoToken_,photonIsoHandle);
    if(!photonIsoHandle.failedToGet())
      isGoodPhotonIsoMap = true;
  }

  if((typeIso_ == "dBetaWeight" and not isGoodNeutralIsoMap) or (typeIso_ == "dBetaWeight" and not isGoodPhotonIsoMap))
    throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] empty maps for neutrals with PFWeight Dbeta correction .. please check \n";

  if((typeIso_ == "puppi" and not isGoodNeutralIsoMap) or (typeIso_ == "puppi" and not isGoodPhotonIsoMap) or (typeIso_ == "puppi" and not isGoodChargeIsoMap))
    throw cms::Exception("Configuration")<<"[patMuonIDIsoSelector] empty maps for puppi isolation correction .. please check \n";

  // Loop on muon collection
  size_t muonIndex = 0;
  pat::MuonCollection::const_iterator itMuon = MuonCollectionHandle->begin();

  auto_ptr<pat::MuonCollection> muonColl(new vector<pat::Muon>);

  for( ; itMuon != MuonCollectionHandle->end(); itMuon++, muonIndex++){

    // apply the pt/eta cut on the best track
    double muonPt = 0;
    reco::Track muonTrack;
    if(itMuon->tunePMuonBestTrack().isNonnull()){
      muonTrack = *(itMuon->tunePMuonBestTrack().get());
      muonPt = muonTrack.pt();
      if(muonPt < ptCut_ or fabs(muonTrack.eta()) > etaCut_) continue;
    }
    else if(itMuon->muonBestTrack().isNonnull()){
      muonTrack = *(itMuon->muonBestTrack().get());
      muonPt = muonTrack.pt();
      if(muonPt < ptCut_ or fabs(muonTrack.eta()) > etaCut_) continue;
    }
    else{
      muonPt = itMuon->pt();
      if (itMuon->pt() < ptCut_ or fabs(itMuon->eta()) > etaCut_) continue;
    }

    // apply standard muon id
    if(typeID_ == "tightID" or typeID_ == "TightID" or typeID_  == "tight" or typeID_ == "Tight"){
      if( muonPt < ptThresholdForHighPt_ ){
	if( not muon::isTightMuon(*itMuon, primaryVertex)) continue;
      }
      else{
	if( not muon::isTightMuon(*itMuon, primaryVertex) and not muon::isHighPtMuon(*itMuon,primaryVertex)) continue;
      }
	  
    }
    else if(typeID_ == "mediumID" or typeID_ == "MediumID" or typeID_  == "medium" or typeID_ == "Medium"){
      if( muonPt < ptThresholdForHighPt_ ){
	if( not muon::isMediumMuon(*itMuon)) continue;
      }
      else{
	if( not muon::isMediumMuon(*itMuon) and not muon::isHighPtMuon(*itMuon,primaryVertex)) continue;
      }
    }
    else if(typeID_ == "looseID" or typeID_ == "LooseID" or typeID_  == "loose" or typeID_ == "Loose"){
      if( muonPt < ptThresholdForHighPt_ ){
	if( not muon::isLooseMuon(*itMuon)) continue;
      }
      else{
	if( not muon::isLooseMuon(*itMuon) and not muon::isHighPtMuon(*itMuon,primaryVertex)) continue;
      }
    }
    else if(typeID_ == "softID" or typeID_ == "SoftID" or typeID_  == "soft" or typeID_ == "Soft"){
      if( not muon::isSoftMuon(*itMuon,primaryVertex)) continue;
    }
    else if(typeID_ == "highPt" or typeID_ == "HightPt"){
      if( not muon::isHighPtMuon(*itMuon,primaryVertex)) continue;
    }

    // isolation
    const pat::MuonRef muonRef(MuonCollectionHandle,muonIndex);

    float chargeIso = 0;
    if(isGoodChargeIsoMap)
      chargeIso = (*chargeHadronIsoHandle)[muonRef];
    else{
      chargeIso = itMuon->userIsolation("PfChargedHadronIso");
    }

    float neutralIso = 0;
    if(isGoodNeutralIsoMap)
      neutralIso = (*neutralHadronIsoHandle)[muonRef];
    else{
      neutralIso = itMuon->userIsolation("PfNeutralHadronIso");
    }

    float photonIso = 0;
    if(isGoodPhotonIsoMap)
      photonIso = (*photonIsoHandle)[muonRef];
    else{
      photonIso = itMuon->userIsolation("PfGammaIso");
    }

    if(typeIso_ == "" or typeIso_ == "dBetaWeight" or typeIso_ == "puppi"){
      if( (chargeIso+neutralIso+photonIso)/muonPt >= relativeIsolationCut_) continue;
    }
    else if(typeIso_ == "dBeta"){
      if( (chargeIso+max(neutralIso+photonIso-0.5*itMuon->userIsolation("PfPUChargedHadronIso"),0.))/muonPt >= relativeIsolationCut_) continue;
    }
    else if(typeIso_ == "rhoCorr"){
      if( (chargeIso+max(neutralIso+photonIso-rho*TMath::Pi()*0.4*0.4,0.))/muonPt >= relativeIsolationCut_) continue;
    }

    pat::Muon newMuon = *(itMuon->clone());
    if(typeIso_ == "dBetaWeight"){
      newMuon.addUserFloat("neutralHadronIsoPFWeight04",neutralIso);
      newMuon.addUserFloat("photonIsoPFWeight04",photonIso);
    }
    else if(typeIso_ == "puppi"){
      newMuon.addUserFloat("neutralHadronIsoPuppi04",neutralIso);
      newMuon.addUserFloat("photonIsoPuppi04",photonIso);
      newMuon.addUserFloat("chargedHadronIsoPuppi04",chargeIso);
    }

    muonColl->push_back(newMuon);
  }

  iEvent.put(muonColl);


}


//______________________________________________________________________________
void patMuonIDIsoSelector::endJob(){}


////////////////////////////////////////////////////////////////////////////////
// plugin definition
////////////////////////////////////////////////////////////////////////////////

DEFINE_FWK_MODULE(patMuonIDIsoSelector);
