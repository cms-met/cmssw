// -*- C++ -*-
//
// Package:    METProducers
// Class:      PFMETProducer
//
//

//____________________________________________________________________________||
#include "RecoMET/METProducers/interface/PFMETProducer.h"

//____________________________________________________________________________||
namespace cms
{

//____________________________________________________________________________||
  PFMETProducer::PFMETProducer(const edm::ParameterSet& iConfig)
    : inputToken_(consumes<edm::View<reco::Candidate> >(iConfig.getParameter<edm::InputTag>("src")))
    , calculateSignificance_(iConfig.getParameter<bool>("calculateSignificance"))
    , globalThreshold_(iConfig.getParameter<double>("globalThreshold"))
  {
    if(calculateSignificance_)
      {
	metSigAlgo_ = new metsig::METSignificance(iConfig);
	
	jetToken_ = mayConsume<edm::View<reco::Jet> >(iConfig.getParameter<edm::InputTag>("srcJets"));
	std::vector<edm::InputTag> srcLeptonsTags = iConfig.getParameter< std::vector<edm::InputTag> >("srcLeptons");
	for(std::vector<edm::InputTag>::const_iterator it=srcLeptonsTags.begin();it!=srcLeptonsTags.end();it++) {
	  lepTokens_.push_back( mayConsume<edm::View<reco::Candidate> >( *it ) );
	}
  
   jetResType_ = iConfig.getParameter<std::string>("srcJetRes");
   rhoToken_ = consumes<double>(iConfig.getParameter<edm::InputTag>("rho"));
      }

    std::string alias = iConfig.exists("alias") ? iConfig.getParameter<std::string>("alias") : "";

    produces<reco::PFMETCollection>().setBranchAlias(alias);
  }

//____________________________________________________________________________||
  void PFMETProducer::produce(edm::Event& event, const edm::EventSetup& setup)
  {
    edm::Handle<edm::View<reco::Candidate> > input;
    event.getByToken(inputToken_, input);

    METAlgo algo;
    CommonMETData commonMETdata = algo.run(*input.product(), globalThreshold_);

    const math::XYZTLorentzVector p4(commonMETdata.mex, commonMETdata.mey, 0.0, commonMETdata.met);
    const math::XYZPoint vtx(0.0, 0.0, 0.0);

    PFSpecificAlgo pf;
    SpecificPFMETData specific = pf.run(*input.product());

    reco::PFMET pfmet(specific, commonMETdata.sumet, p4, vtx);

    if(calculateSignificance_)
      {
	reco::METCovMatrix sigcov = getMETCovMatrix(event, setup, *input);
	pfmet.setSignificanceMatrix(sigcov);
      }

    std::auto_ptr<reco::PFMETCollection> pfmetcoll;
    pfmetcoll.reset(new reco::PFMETCollection);

    pfmetcoll->push_back(pfmet);
    event.put(pfmetcoll);
  }



  reco::METCovMatrix PFMETProducer::getMETCovMatrix(const edm::Event& event, const edm::EventSetup& setup, const edm::View<reco::Candidate>& candInput) const {

	// leptons
	std::vector< edm::Handle<reco::CandidateView> > leptons;
	for ( std::vector<edm::EDGetTokenT<edm::View<reco::Candidate> > >::const_iterator srcLeptons_i = lepTokens_.begin();
	      srcLeptons_i != lepTokens_.end(); ++srcLeptons_i ) {
	  edm::Handle<reco::CandidateView> leptons_i;
	  event.getByToken(*srcLeptons_i, leptons_i);
     leptons.push_back( leptons_i );
     /*
	  for ( reco::CandidateView::const_iterator lepton = leptons_i->begin();
		lepton != leptons_i->end(); ++lepton ) {
	    leptons.push_back(*lepton);
	  }
     */
	}

	// jets
	edm::Handle<edm::View<reco::Jet> > inputJets;
	event.getByToken( jetToken_, inputJets );

   JME::JetResolution resObj = JME::JetResolution::get(setup, jetResType_);
   JME::JetResolutionScaleFactor resSFObj = JME::JetResolutionScaleFactor::get(setup, jetResType_);

   edm::Handle<double> rho;
   event.getByToken(rhoToken_, rho);

	//Compute the covariance matrix and fill it
	reco::METCovMatrix cov = metSigAlgo_->getCovariance( *inputJets, leptons, candInput, *rho, resObj, resSFObj);

	return cov;
  }





//____________________________________________________________________________||
  DEFINE_FWK_MODULE(PFMETProducer);
}

//____________________________________________________________________________||
