#include <iostream>

#include "RecoMET/METPUSubtraction/plugins/MVAMET.h"
#include "FWCore/Framework/interface/MakerMacros.h"

MVAMET::MVAMET(const edm::ParameterSet& cfg){

  saveMap_ = cfg.getParameter<bool>("saveMap");
  if(saveMap_)
  {
    produces<std::vector<std::string>>();
    produces<std::vector<Float_t>>();
  }
  // get tokens for input METs and prepare for saving the corresponding recoils to the event
  srcMETTags_   = cfg.getParameter<vInputTag>("srcMETs");
  for(vInputTag::const_iterator it=srcMETTags_.begin();it!=srcMETTags_.end();it++) {
    srcMETs_.push_back( consumes<pat::METCollection >( *it ) );
  }

  
  // take flags for the met
  srcMETFlags_ = cfg.getParameter<std::vector<int>>("inputMETFlags");
  
  if(srcMETFlags_.size() != srcMETTags_.size())
    throw cms::Exception("MVAMET::MVAMET") << " Failed to load MET flags   !!\n" << "Expected " << srcMETTags_.size() << " but got " << srcMETFlags_.size() << std::endl;

  //get leptons to calculate Z vector
  vInputTag srcLeptonsTags = cfg.getParameter<vInputTag>("srcLeptons");
  for(vInputTag::const_iterator it=srcLeptonsTags.begin();it!=srcLeptonsTags.end();it++) {
    srcLeptons_.push_back( consumes<reco::CandidateView >( *it ) );
  }
    
  debug_ = (cfg.existsAs<bool>("debug")) ? cfg.getParameter<bool>("debug") : false;
  combineNLeptons_ = cfg.getParameter<int>("combineNLeptons");
  requireOS_ = cfg.getParameter<bool>("requireOS");
  // check config: only when exactly two leptons are combined, OS requirement makes sense
  if(requireOS_)
    assert( combineNLeptons_ == 2 );
  useTauSig_    = cfg.getParameter<bool>("useTauSig");

  srcVertices_  = consumes<reco::VertexCollection>(cfg.getParameter<edm::InputTag>("srcVertices"));
  srcJets_      = consumes<pat::JetCollection>(cfg.getParameter<edm::InputTag>("srcJets"));
  srcTaus_      = consumes<pat::TauCollection>(cfg.getParameter<edm::InputTag>("srcTaus"));
  srcMuons_     = consumes<pat::MuonCollection>(cfg.getParameter<edm::InputTag>("srcMuons"));
  if(useTauSig_)
    srcTausSignificance_ = consumes<math::Error<2>::type>(cfg.getParameter<edm::InputTag>("tausSignificance"));

  permuteLeptonsWithinPlugin_ = cfg.getParameter<bool>("permuteLeptonsWithinPlugin");
  if (!permuteLeptonsWithinPlugin_)
    leptonPermutationsHandle_ = consumes<reco::CompositeCandidateCollection>(cfg.getParameter<edm::InputTag>("leptonPermutations"));
  
  // load weight files
  edm::FileInPath weightFile; 
  weightFile = cfg.getParameter<edm::FileInPath>("weightFile");
  mvaReaderPhiCorrection_     = loadMVAfromFile(weightFile, variablesForPhiTraining_, "PhiCorrectedRecoil"); 
  mvaReaderRecoilCorrection_  = loadMVAfromFile(weightFile, variablesForRecoilTraining_, "LongZCorrectedRecoil");
  mvaReaderCovU1_             = loadMVAfromFile(weightFile, variablesForCovU1_, "CovU1");
  mvaReaderCovU2_             = loadMVAfromFile(weightFile, variablesForCovU2_, "CovU2");

  // prepare for saving the final mvaMET to the event
  mvaMETLabel_ = cfg.getParameter<std::string>("MVAMETLabel");
  produces<pat::METCollection>(mvaMETLabel_);
}

MVAMET::~MVAMET(){
  delete mvaReaderPhiCorrection_;
  delete mvaReaderRecoilCorrection_;
  delete mvaReaderCovU1_;
  delete mvaReaderCovU2_;
}

metPlus MVAMET::calculateRecoil(metPlus* MET, const recoilingBoson &Z, edm::Event& evt)
{
    reco::METCovMatrix rotateToZFrame;
    auto Zp4 = Z.p4vec();
    rotateToZFrame(0,0) = rotateToZFrame(1,1) = std::cos(- Zp4.Phi());
    rotateToZFrame(0,1) =   std::sin(- Zp4.Phi());
    rotateToZFrame(1,0) = - std::sin(- Zp4.Phi());

    metPlus Recoil((*MET)); 
    Recoil.setP4( - Recoil.p4() );
    if( MET->containsCharged() )
    {
      Recoil.setP4(Recoil.p4() - Z.chargedP4());
      Recoil.setSumEt(Recoil.sumEt()-Z.chargedSumEt());    
    }
 
    if( MET->containsNeutral() )
      Recoil.setSumEt(Recoil.sumEt()-Z.neutralSumEt());

    reco::METCovMatrix rotatedCovMatrix = rotateToZFrame * Recoil.getSignificanceMatrix();
    Recoil.setSignificanceMatrix( rotatedCovMatrix );

    return Recoil;
}

void MVAMET::doCombinations(int offset, int k)
{ 
  if (k == 0)
  {
    combinations_.push_back(combination_);
    combination_.pop_back();
    return;
  }
  for (size_t i = offset; i <= allLeptons_.size() - k; ++i)
  {
    combination_.push_back(allLeptons_[i]);
    doCombinations(i+1, k-1);
  }
  combination_.clear();
}

void MVAMET::unpackCompositeCands(edm::Event& evt)
{
  edm::Handle<reco::CompositeCandidateCollection> ccands;
  evt.getByToken(leptonPermutationsHandle_, ccands);
  for (auto ccand = ccands->begin(); ccand != ccands->end(); ++ccand)
  {
    combination_.clear();
    for (unsigned int ielem = 0; ielem < combineNLeptons_; ++ielem)
    { 
      const reco::Candidate& cand = *(ccand->daughter(ielem));
      combination_.push_back(cand.masterClone().castTo<edm::Ptr<reco::Candidate> >());
    }
    combinations_.push_back(combination_);
  }
}

void MVAMET::handleTaus(edm::Ptr<reco::Candidate> lepton, recoilingBoson& Z, const pat::TauCollection& tauCollection)
{
  recoilComponent rComp(lepton);
  for(const auto & tau : tauCollection)
  {
    if(deltaR2 (*lepton, tau) > 1.e-6) // dR > 0.001
      continue;
    for(const auto & candidate : tau.signalCands())
    {
      if(abs(candidate->pdgId()) > 11 and abs(candidate->pdgId()) < 16)
        continue;

      if(candidate->charge() !=0)
        rComp.chargedTauJetCandidates.push_back(candidate);
      else
        rComp.neutralTauJetCandidates.push_back(candidate);
    }
  }
  Z.addLepton(rComp);
}

void MVAMET::handleMuons(edm::Ptr<reco::Candidate> lepton, recoilingBoson& Z, const pat::MuonCollection& muCollection)
{
  math::PtEtaPhiELorentzVectorD p4Photon;      
  recoilComponent rComp(lepton); 
  for (const auto & muon : muCollection)
  {
    if(deltaR2 (*lepton, muon) < 1.e-6) // dR < 0.001
    {
      p4Photon.SetPt(muon.pfEcalEnergy()/TMath::CosH(muon.p4().eta()));
      p4Photon.SetEta(muon.p4().eta());
      p4Photon.SetPhi(muon.p4().phi());
      p4Photon.SetE(muon.pfEcalEnergy());
    }
  }

  if(p4Photon.E() > 0 )
    rComp.setP4(lepton->p4() + p4Photon);
  else
    rComp.setP4(lepton->p4());

  Z.addLepton(rComp);
}

void MVAMET::calculateRecoilingObjects(edm::Event &evt, const pat::MuonCollection& muCollection, const pat::TauCollection& tauCollection)
{
  // loop on all the indentified leptons and put them in a common vector allLeptons_
  allLeptons_.clear();
  combinations_.clear();
  for ( std::vector<edm::EDGetTokenT<reco::CandidateView > >::const_iterator srcLeptons_i = srcLeptons_.begin(); srcLeptons_i != srcLeptons_.end(); ++srcLeptons_i )
  {
    edm::Handle<reco::CandidateView> leptons;
    evt.getByToken(*srcLeptons_i, leptons);
    for (size_t i=0; i < leptons->size(); ++i)
      allLeptons_.push_back(leptons->ptrAt(i));
  }

  if (!permuteLeptonsWithinPlugin_)
    unpackCompositeCands(evt);

  else if(allLeptons_.size() >= combineNLeptons_)
    doCombinations(0, combineNLeptons_);

  if(debug_)
    std::cout << allLeptons_.size() << " lead to " << combinations_.size();

  if(requireOS_)
    cleanLeptonsFromSS();

  if(debug_)
    std::cout << " with OS: " << combinations_.size() << std::endl;

  for(auto leptonpair: combinations_)
  {
    recoilingBoson Z;
    for(auto lepton : leptonpair)
    {
      if(abs(lepton->pdgId()) == 13)
        handleMuons( lepton, Z, muCollection);
      else if(abs(lepton->pdgId()) == 15)
        handleTaus( lepton, Z, tauCollection);
      else if(abs(lepton->pdgId())==11)
        {
          recoilComponent rComp(lepton);
          rComp.setP4(lepton->p4());
          Z.addLepton(rComp);
        }
      else
        std::cout << "Warning: unsupported recoiling object found" << std::endl;
    }
    Bosons_.push_back(Z);
  } 
}

void MVAMET::fillEventInformation(edm::Event& evt)
{
  edm::Handle<pat::JetCollection> jets;
  evt.getByToken(srcJets_, jets);
  size_t jetsSize = jets->size();
  for( size_t iJet = 0; iJet <= 1; iJet++)
  {
    var_["Jet" + std::to_string(iJet)+ "_Pt"]  = (jetsSize > iJet) ? jets->at(iJet).p4().pt() : 0;
    var_["Jet" + std::to_string(iJet)+ "_Eta"] = (jetsSize > iJet) ? jets->at(iJet).p4().Eta() : 0;
    var_["Jet" + std::to_string(iJet)+ "_Phi"] = (jetsSize > iJet) ? jets->at(iJet).p4().Phi() : 0;
    var_["Jet" + std::to_string(iJet)+ "_M"]   = (jetsSize > iJet) ? jets->at(iJet).p4().M() : 0;
  }

  var_["NCleanedJets"] = countJets(*jets, 10);
  var_["nCombinations"] = combinations_.size();

  // treat other collections and save to map
  edm::Handle<reco::VertexCollection> vertices;
  evt.getByToken(srcVertices_, vertices);
  var_["NVertex"] = vertices->size();
}

void MVAMET::TagZ()
{
  if(Bosons_.size() == 0)
   return;
  
  std::vector<recoilingBoson>::iterator taggedBoson = Bosons_.begin(); 
  for(std::vector<recoilingBoson>::iterator Z= Bosons_.begin(); Z != Bosons_.end(); Z++)
  {
    if(taggedBoson->p4vec().pt() < Z->p4vec().pt())
      taggedBoson = Z;
  }
  taggedBoson->setTagged();
}

void MVAMET::produce(edm::Event& evt, const edm::EventSetup& es){
  var_.clear();
  Bosons_.clear();

  // get taus
  edm::Handle<pat::TauCollection> tauCollectionHandle;
  evt.getByToken(srcTaus_, tauCollectionHandle);
  const pat::TauCollection tauCollection = *(tauCollectionHandle.product());

  // get muons
  edm::Handle<pat::MuonCollection> muCollectionHandle;
  evt.getByToken(srcMuons_, muCollectionHandle);
  const pat::MuonCollection muCollection = *(muCollectionHandle.product());

  // read in additional taus significance contribution
  edm::Handle<math::Error<2>::type> tausSignificance;
  reco::METCovMatrix tausMatrix; 
  if(useTauSig_)
  {
    evt.getByToken(srcTausSignificance_, tausSignificance);
    tausMatrix(0,0) = (*tausSignificance)(0,0);
    tausMatrix(1,0) = (*tausSignificance)(1,0);
    tausMatrix(0,1) = (*tausSignificance)(0,1);
    tausMatrix(1,1) = (*tausSignificance)(1,1);
  }

  //fill allLeptons_
  calculateRecoilingObjects(evt, muCollection, tauCollection);

  //fill event meta information
  fillEventInformation(evt);
  // create output collections
  std::auto_ptr<pat::METCollection> recoilpatMETCollection(new pat::METCollection());
  std::auto_ptr<pat::METCollection> patMETCollection(new pat::METCollection());

  // stop execution if no recoiling object has been found
  if(Bosons_.size() == 0)
  {
    if(saveMap_)
      saveMap(evt);
    evt.put(patMETCollection,mvaMETLabel_);
    return;
  }

  if(saveMap_)
    TagZ();

  std::vector<edm::EDGetTokenT<pat::METCollection> >::const_iterator referenceMET = srcMETs_.begin();
  evt.getByToken(*referenceMET, referenceMETHandle_);
  // loop on identified combinations of recoiling objects, here donted as "Z" 
  for(const auto & Z: Bosons_)
  {
    metPlus referenceRecoil;
    std::vector<int>::const_iterator itMETFlags = srcMETFlags_.begin();

    // MET flags show what components have to be subtracted to get the recoil from the MET
    // MET Flags: 0 -> Neutral + Charged PV
    //            1 -> only charged PV
    //            2 -> only Neutral PV
    //            3 -> not included in MET
    //
    int i = 0;
    for ( std::vector<edm::EDGetTokenT<pat::METCollection> >::const_iterator srcMET = srcMETs_.begin(); srcMET != srcMETs_.end() && itMETFlags!=srcMETFlags_.end(); ++srcMET, ++itMETFlags )
    {
      edm::Handle<pat::METCollection> METhandle;
      evt.getByToken(*srcMET, METhandle);
      assert((*METhandle).size() == 1 );

      metPlus MET((*METhandle)[0]);
      MET.collection_name = srcMETTags_[i].label();
      MET.METFlag = (*itMETFlags);

      // to be removed begin
      if(i==0)
      {
        if(saveMap_)
        {
          genMET_ = (*METhandle)[0].genMET();
          var_["genMet_Pt"] = genMET_->p4().pt();
          var_["genMet_Phi"] = genMET_->p4().phi();
        }
      }
      // to be removed end

      auto Recoil = calculateRecoil(&MET, Z, evt);
      if(i == 0)
        referenceRecoil = Recoil;

      addToMap(Recoil, referenceRecoil);
      if(saveMap_)
        addToMap(Recoil, Z);
      ++i;
    }

    // evaluate phi training and apply angular correction
    Float_t PhiAngle = GetResponse(mvaReaderPhiCorrection_, variablesForPhiTraining_);

    auto refRecoil = TVector2(referenceRecoil.p4().px(), referenceRecoil.p4().py());
    refRecoil = refRecoil.Rotate(PhiAngle);
    reco::Candidate::LorentzVector phiCorrectedRecoil(refRecoil.Px(), refRecoil.Py(), 0, referenceRecoil.sumEt());
    addToMap(phiCorrectedRecoil, referenceRecoil.sumEt(), "PhiCorrectedRecoil");
  
    // evaluate second training and apply recoil correction
    Float_t RecoilCorrection = GetResponse(mvaReaderRecoilCorrection_, variablesForRecoilTraining_);
    refRecoil *= RecoilCorrection;

    // create pat::MET from recoil
    pat::MET recoilmvaMET(referenceRecoil);
    reco::Candidate::LorentzVector recoilP4(refRecoil.Px(), refRecoil.Py(), 0, referenceRecoil.sumEt());
    recoilmvaMET.setP4(recoilP4);
    addToMap(recoilP4, referenceRecoil.sumEt(), "LongZCorrectedRecoil");

    // evaluate covariance matrix regression
    Float_t CovU1Correction = GetResponse(mvaReaderCovU1_, variablesForCovU1_);
    Float_t CovU1 = CovU1Correction * refRecoil.Mod();
    Float_t CovU2Correction = GetResponse(mvaReaderCovU2_, variablesForCovU2_);
    Float_t CovU2 = CovU2Correction * refRecoil.Mod();
  
    //// save results to event
    recoilpatMETCollection->push_back(recoilmvaMET);

    // calculate new mvaMET
    pat::MET mvaMET(referenceRecoil);
    mvaMET.setP4(recoilP4);
    mvaMET.setP4(mvaMET.p4() + Z.chargedP4());
    mvaMET.setP4(- mvaMET.p4());
    // copy sumEt from input MET since there's no correction in MVA MET for that
    mvaMET.setSumEt((*referenceMETHandle_)[0].sumEt());
    reco::METCovMatrix mvaMETCov;
    double cosPhi =  std::cos(refRecoil.Phi());
    double sinPhi =  std::sin(refRecoil.Phi());
    mvaMETCov(0, 0) =  std::pow(CovU1, 2)*cosPhi*cosPhi + std::pow(CovU2, 2)*sinPhi*sinPhi;
    mvaMETCov(0, 1) = -std::pow(CovU1, 2) * sinPhi*cosPhi + std::pow(CovU2, 2) * sinPhi*cosPhi;
    mvaMETCov(1, 0) =  mvaMETCov(0, 1);
    mvaMETCov(1, 1) =  std::pow(CovU1, 2) * sinPhi*sinPhi + std::pow(CovU2, 2) * cosPhi*cosPhi;

    if(useTauSig_)
      mvaMET.setSignificanceMatrix(mvaMETCov + tausMatrix);
    else
      mvaMET.setSignificanceMatrix(mvaMETCov);

    // add constituent info to pat::MET ; only if combinatorics done here (else potential problems with edm::Ptr cast)
    if (permuteLeptonsWithinPlugin_)
    {
      size_t iCount=0;
      for(const auto & lepton: Z.leptons)
        mvaMET.addUserCand("lepton" + std::to_string(iCount++), lepton.getSrcLepton());
    }

    // save MVA MET results
    mvaMET.addUserFloat("PhiCorrection", PhiAngle);
    mvaMET.addUserFloat("LongZCorrection", RecoilCorrection);
    mvaMET.addUserFloat("CovU1", CovU1Correction);
    mvaMET.addUserFloat("CovU2", CovU2Correction);
    patMETCollection->push_back(mvaMET);
   
   // muon selection for training
    if(saveMap_)
    {
      var_["PhiTrainingResponse"] = PhiAngle;
      var_["RecoilTrainingResponse"] = RecoilCorrection;
      var_["MVAMET_Pt"] = mvaMET.p4().pt();
      var_["MVAMET_Phi"] = mvaMET.p4().phi() ;
      var_["MVAMET_sumEt"] = mvaMET.sumEt();
   
      reco::Candidate::LorentzVector dmvamet = genMET_->p4() - mvaMET.p4();
      var_["dmvamet_Pt"] =    dmvamet.Pt();
      var_["dmvamet_Phi"] =   dmvamet.Phi();
      reco::Candidate::LorentzVector dpfmet = genMET_->p4() - (*referenceMETHandle_)[0].p4();
      var_["dpfmet_Pt"] =    dpfmet.Pt();
      var_["dpfmet_Phi"] =   dpfmet.Phi();
      if(Z.isTagged())
      {
        addToMap(Z);
        var_["select"] = Z.select();
        saveMap(evt);
      }
    }
  }
  if(debug_)
  {
   for(const auto & entry : var_)
     std::cout << entry.first << " : " << entry.second << std::endl;
  }
  evt.put(patMETCollection, mvaMETLabel_);
}

void MVAMET::saveMap(edm::Event& evt)
{
  std::auto_ptr<std::vector<std::string>> variableNames(new std::vector<std::string>);
  std::auto_ptr<std::vector<Float_t> > variables(new std::vector<Float_t>);

  for(const auto & entry : var_){
    variableNames->push_back(entry.first);
    variables->push_back(entry.second);
  }

  evt.put(variableNames);
  evt.put(variables);
}

void MVAMET::addToMap(const metPlus &recoil, const recoilingBoson &Z)
{
  reco::Candidate::LorentzVector p4(Z.chargedP4() );
  TVector2 diLeptonMomentum(p4.X(), p4.Y());
  TVector2 recoilT2(recoil.p4().X(), recoil.p4().Y());
  TVector2 rotatedRecoil = recoilT2.Rotate(- diLeptonMomentum.Phi());
  auto type = "recoil" + recoil.collection_name;
  var_[type + "_LongZ" ] = rotatedRecoil.X();
  var_[type + "_PerpZ" ] = rotatedRecoil.Y();

}

void MVAMET::addToMap(const metPlus &recoil, const metPlus &referenceMET)
{
  auto type = "recoil" + recoil.collection_name;
  addToMap(recoil.p4(), recoil.sumEt(), type);
  var_[type +  "_Cov00" ] = recoil.getSignificanceMatrix()(0,0);
  var_[type +  "_Cov11" ] = recoil.getSignificanceMatrix()(1,1);
  var_[type + "_sumEtFraction" ] = recoil.sumEt()/referenceMET.sumEt();
  var_[type + "_dPhi" ] = TVector2::Phi_mpi_pi(recoil.phi() - referenceMET.phi());
}

void MVAMET::addToMap(const recoilingBoson &Z)
{
  reco::Candidate::LorentzVector Zp4 = Z.chargedP4();
  var_["Boson_Pt"] = Zp4.pt();
  var_["Boson_Phi"] = Zp4.Phi();
  var_["Boson_Eta"] = Zp4.Eta();
  var_["Boson_M"] = Zp4.M();
  var_["Boson_sumET"] = Z.chargedSumEt();
  var_["Boson_daughter1"] = float(Z.getPdgId(0));
  var_["Boson_daughter2"] = float(Z.getPdgId(1));
  
}

void MVAMET::addToMap(const reco::Candidate::LorentzVector p4, const double sumEt, const std::string &type)
{
  var_[type + "_Pt" ] = p4.pt();
  var_[type + "_Phi" ] = p4.phi();
  var_[type + "_sumEt" ] = sumEt;
}

unsigned int MVAMET::countJets(const pat::JetCollection& jets, const float maxPt){
  int nJets = 0;
  for(const auto & jet : jets){
    if(jet.pt() > maxPt)
      nJets++;
  }
  return nJets;
}

const Float_t MVAMET::GetResponse(const GBRForest * Reader, std::vector<std::string> &variableNames ){

  Float_t* mvaInputVector = createFloatVector(variableNames);
  double result = Reader->GetResponse(mvaInputVector);
  delete mvaInputVector;
  return result;
}


const GBRForest* MVAMET::loadMVAfromFile(const edm::FileInPath& inputFileName, std::vector<std::string>& trainingVariableNames, std::string mvaName){
  
  if ( inputFileName.location()==edm::FileInPath::Unknown ) 
    throw cms::Exception("PFMETAlgorithmMVA::loadMVA") << " Failed to find File = " << inputFileName << " !!\n";
  
  std::unique_ptr<TFile> inputFile(new TFile(inputFileName.fullPath().data()));
  std::string variableListName = mvaName + "varlist";
  std::unique_ptr<std::vector<std::string>> lVec ((std::vector<std::string>*)inputFile->Get(variableListName.c_str()));

  for(unsigned int i=0; i< lVec->size();++i)
  {
    trainingVariableNames.push_back(lVec->at(i));
  }
  const GBRForest* mva = (GBRForest*)inputFile->Get(mvaName.data());
  if ( !mva )
    throw cms::Exception("PFMETAlgorithmMVA::loadMVA") << " Failed to load MVA from file = " << inputFileName.fullPath().data() << " !!\n";
  
  return mva;
}

Float_t* MVAMET::createFloatVector(std::vector<std::string> variableNames){
  Float_t* floatVector = new Float_t[variableNames.size()];
  for(size_t i = 0; i < variableNames.size(); ++i){
      floatVector[i] = var_[variableNames[i]];
  }
  return floatVector;
}


DEFINE_FWK_MODULE(MVAMET);
