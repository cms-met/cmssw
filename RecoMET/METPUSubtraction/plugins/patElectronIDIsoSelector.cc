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
#include "DataFormats/PatCandidates/interface/Electron.h"
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
class patElectronIDIsoSelector : public edm::stream::EDProducer<> {

public:
  // construction/destruction
  patElectronIDIsoSelector(const edm::ParameterSet& iConfig);
  ~patElectronIDIsoSelector() {;}
  
  // member functions
  void produce(edm::Event& iEvent,const edm::EventSetup& iSetup);
  void endJob();

private:
  // input electron collection to be considered
  edm::InputTag src_;
  // rho collection in case of effective area
  edm::InputTag rho_;
  // vertex collection
  edm::InputTag vertex_;

  // value map with re-calcuated isolations from iso-deposits .. different wrt to the ones already inside the muon object
  edm::InputTag charged_hadron_iso_;
  edm::InputTag neutral_hadron_iso_;
  edm::InputTag photon_iso_;

  // input tag with the value map for the electron id to be applied
  edm::InputTag electron_id_;

  // isolation, pt and eta cut to be applied
  double relativeIsolationCut_;
  double ptCut_;
  double etaCut_;

  // type of relative isolation cut
  std::string typeIso_;

  // tokens
  edm::EDGetTokenT<pat::ElectronCollection> srcToken_ ;
  edm::EDGetTokenT<double>                  rhoToken_ ;
  edm::EDGetTokenT<reco::VertexCollection>  vertexToken_ ;

  edm::EDGetTokenT<edm::ValueMap<bool> > electron_idToken_;

  edm::EDGetTokenT<edm::ValueMap<double> > charged_hadron_isoToken_ ;
  edm::EDGetTokenT<edm::ValueMap<double> > neutral_hadron_isoToken_ ;
  edm::EDGetTokenT<edm::ValueMap<double> > photon_isoToken_ ;

};




////////////////////////////////////////////////////////////////////////////////
// construction/destruction
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
patElectronIDIsoSelector::patElectronIDIsoSelector(const edm::ParameterSet& iConfig){

  
  if(iConfig.existsAs<edm::InputTag >("src")){
    src_ = iConfig.getParameter<edm::InputTag>("src");
    if(src_ == edm::InputTag(""))
      throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] empty electron tag is given --> please check \n"; 
  }
  else throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] no electron tag is given \n";  

  if(iConfig.existsAs<edm::InputTag >("vertex")){
    vertex_ = iConfig.getParameter<edm::InputTag>("vertex");
    if(vertex_ == edm::InputTag(""))
      throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] empty vertex tag is given --> please check \n"; 
  }
  else throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] no vertex tag is given \n";  

  if(iConfig.existsAs<edm::InputTag >("rho"))
    rho_ = iConfig.getParameter<edm::InputTag>("rho");
  

  if(iConfig.existsAs<edm::InputTag >("charged_hadron_iso"))
    charged_hadron_iso_ = iConfig.getParameter<edm::InputTag>("charged_hadron_iso");
  

  if(iConfig.existsAs<edm::InputTag >("neutral_hadron_iso"))
    neutral_hadron_iso_ = iConfig.getParameter<edm::InputTag>("neutral_hadron_iso");
  

  if(iConfig.existsAs<edm::InputTag >("photon_iso"))
    photon_iso_ = iConfig.getParameter<edm::InputTag>("photon_iso");

  if(iConfig.existsAs<edm::InputTag >("electron_id")){
    electron_id_ = iConfig.getParameter<edm::InputTag>("electron_id");
    if(electron_id_ == edm::InputTag(""))
      throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] no electron id value map provided --> please check \n"; 
  }
  else throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] no electron id value map is given \n";  

  if(iConfig.existsAs<double>("relativeIsolationCut"))
    relativeIsolationCut_ = iConfig.getParameter<double>("relativeIsolationCut");

  if(iConfig.existsAs<double>("ptCut"))
    ptCut_ = iConfig.getParameter<double>("ptCut");

  if(iConfig.existsAs<double>("etaCut"))
    etaCut_ = iConfig.getParameter<double>("etaCut");
  
  if(iConfig.existsAs<std::string>("typeIso"))
    typeIso_ = iConfig.getParameter<std::string>("typeIso");

  // tokens
  if(!(src_ == edm::InputTag(""))) 
    srcToken_ = consumes<pat::ElectronCollection>(src_);

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

  if(!(electron_id_ == edm::InputTag(""))) 
    electron_idToken_ = consumes<edm::ValueMap<bool> >(electron_id_);


   produces<pat::ElectronCollection>();
}


////////////////////////////////////////////////////////////////////////////////
// implementation of member functions
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
void patElectronIDIsoSelector::produce(edm::Event& iEvent,const edm::EventSetup& iSetup){

  edm::Handle<pat::ElectronCollection> ElectronCollectionHandle;
  if(!(src_ == edm::InputTag(""))){  
    iEvent.getByToken(srcToken_,ElectronCollectionHandle);
    if(ElectronCollectionHandle.failedToGet()){
      throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] failed to get electron collection \n"; 
    }
  }    

  edm::Handle<reco::VertexCollection> VertexCollectionHandle;
  if(!(vertex_ == edm::InputTag(""))) { 
    iEvent.getByToken(vertexToken_,VertexCollectionHandle);
    if(VertexCollectionHandle.failedToGet()) {
      throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] failed to get vertex collection \n"; 
    }
  }


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


  // take the value map for ele ID
  bool isGoodEleValueMap = false;
  edm::Handle<edm::ValueMap<bool>> electronIDHandle;
  if(!(electron_id_ == edm::InputTag(""))){
    iEvent.getByToken(electron_idToken_,electronIDHandle);
    if(!electronIDHandle.failedToGet())
      isGoodEleValueMap = true;
  }

  if(isGoodEleValueMap == false)
    throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] failed to get the value map for ele id .. please check \n";

  if((typeIso_ == "dBetaWeight" and not isGoodNeutralIsoMap) or (typeIso_ == 3 and not isGoodPhotonIsoMap))
    throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] empty maps for neutrals with PFWeight Dbeta correction .. please check \n";

  if((typeIso_ == "puppi" and not isGoodNeutralIsoMap) or (typeIso_ == "puppi" and not isGoodPhotonIsoMap) or (typeIso_ == "puppi" and not isGoodChargeIsoMap))
    throw cms::Exception("Configuration")<<"[patElectronIDIsoSelector] empty maps for puppi isolation correction .. please check \n";

  // Loop on muon collection
  size_t electronIndex = 0;
  pat::ElectronCollection::const_iterator itElectron = ElectronCollectionHandle->begin();

  auto_ptr<pat::ElectronCollection> electronColl(new vector<pat::Electron>);

  for( ; itElectron != ElectronCollectionHandle->end(); itElectron++, electronIndex++){

    // apply the pt/eta cut on the best track
    double electronPt = 0;
    reco::GsfTrack electronTrack;
    if(itElectron->gsfTrack().isNonnull()){
      electronTrack = *(itElectron->gsfTrack().get());
      electronPt = electronTrack.pt();
      if(electronPt < ptCut_ or fabs(electronTrack.eta()) > etaCut_) continue;
    }
    else{
      electronPt = itElectron->pt();
      if (itElectron->pt() < ptCut_ or fabs(itElectron->eta()) > etaCut_) continue;
    }

    // apply the id starting from the value map
    const pat::ElectronRef electronRef(ElectronCollectionHandle,electronIndex);

    if(not (*electronIDHandle)[electronRef]) continue;

    // isolation

    float chargeIso = 0;
    if(isGoodChargeIsoMap)
      chargeIso = (*chargeHadronIsoHandle)[electronRef];
    else{
      chargeIso = itElectron->userIsolation("PfChargedHadronIso");
    }

    float neutralIso = 0;
    if(isGoodNeutralIsoMap)
      neutralIso = (*neutralHadronIsoHandle)[electronRef];
    else{
      neutralIso = itElectron->userIsolation("PfNeutralHadronIso");
    }

    float photonIso = 0;
    if(isGoodPhotonIsoMap)
      photonIso = (*photonIsoHandle)[electronRef];
    else{
      photonIso = itElectron->userIsolation("PfGammaIso");
    }

    if(typeIso_ == "" or typeIso_ == "dBetaWeight" or typeIso_ == "puppi"){
      if( (chargeIso+neutralIso+photonIso)/electronPt >= relativeIsolationCut_) continue;
    }
    else if(typeIso_ == "dBeta"){
      if( (chargeIso+max(neutralIso+photonIso-0.5*itElectron->userIsolation("PfPUChargedHadronIso"),0.))/electronPt >= relativeIsolationCut_) continue;
    }
    else if(typeIso_ == ""){
      if( (chargeIso+max(neutralIso+photonIso-rho*TMath::Pi()*0.4*0.4,0.))/electronPt >= relativeIsolationCut_) continue;
    }

    pat::Electron newElectron = *(itElectron->clone());
    if(typeIso_ == 3){
      newElectron.addUserFloat("neutralHadronIsoPFWeight03",neutralIso);
      newElectron.addUserFloat("photonIsoPFWeight03",photonIso);
    }
    else if(typeIso_ == 4){
      newElectron.addUserFloat("neutralHadronIsoPuppi03",neutralIso);
      newElectron.addUserFloat("photonIsoPuppi03",photonIso);
      newElectron.addUserFloat("chargedHadronIsoPuppi03",chargeIso);
    }

    electronColl->push_back(newElectron);
  }

  iEvent.put(electronColl);


}


//______________________________________________________________________________
void patElectronIDIsoSelector::endJob(){}


////////////////////////////////////////////////////////////////////////////////
// plugin definition
////////////////////////////////////////////////////////////////////////////////

DEFINE_FWK_MODULE(patElectronIDIsoSelector);
