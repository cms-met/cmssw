import FWCore.ParameterSet.Config as cms

##____________________________________________________________________________||
process = cms.Process("TEST")

##____________________________________________________________________________||
process.load("FWCore.MessageLogger.MessageLogger_cfi")

##____________________________________________________________________________||
process.load("RecoMET/METProducers.PFMET_cfi")
process.load("RecoMET/METProducers.METSignificanceParams_cfi")
process.load("RecoMET/METProducers.METSignificanceObjects_cfi")

##____________________________________________________________________________||
process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
       "/store/relval/CMSSW_7_3_0_pre1/RelValZEE_13/GEN-SIM-RECO/PRE_LS172_V15-v1/00000/C2212C6F-EB59-E411-AB78-0025905B85EE.root",
       "/store/relval/CMSSW_7_3_0_pre1/RelValZEE_13/GEN-SIM-RECO/PRE_LS172_V15-v1/00000/F0DAE66D-EB59-E411-95AF-0025905A6088.root"
       )
    )

##____________________________________________________________________________||
process.out = cms.OutputModule(
    "PoolOutputModule",
    fileName = cms.untracked.string('recoMET_pfMet.root'),
    SelectEvents   = cms.untracked.PSet( SelectEvents = cms.vstring('p') ),
    outputCommands = cms.untracked.vstring(
        'drop *',
        'keep *_*_*_TEST'
        )
    )

##____________________________________________________________________________||
process.options   = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))
process.MessageLogger.cerr.FwkReport.reportEvery = 50
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(10))

##____________________________________________________________________________||
process.load('Configuration.StandardSequences.Services_cff')
process.load("JetMETCorrections.Modules.JetResolutionESProducer_cfi")
from CondCore.DBCommon.CondDBSetup_cfi import *

process.jer = cms.ESSource("PoolDBESSource",
      CondDBSetup,
      toGet = cms.VPSet(
         # Pt Resolution
         cms.PSet(
            record = cms.string('JetResolutionRcd'),
            tag    = cms.string('JR_MC_PtResolution_Summer15_25nsV6_AK4PFchs'),
            label  = cms.untracked.string('AK4PFchs_pt')
            ),

         # Phi Resolution
         cms.PSet(
            record = cms.string('JetResolutionRcd'),
            tag    = cms.string('JR_MC_PhiResolution_Summer15_25nsV6_AK4PFchs'),
            label  = cms.untracked.string('AK4PFchs_phi')
            ),

         # Scale factors
         cms.PSet(
            record = cms.string('JetResolutionScaleFactorRcd'),
            tag    = cms.string('JR_DATAMCSF_Summer15_25nsV6_AK4PFchs'),
            label  = cms.untracked.string('AK4PFchs')
            ),
         ),
      connect = cms.string('sqlite:Summer15_25nsV6.db')
      )

process.es_prefer_jer = cms.ESPrefer('PoolDBESSource', 'jer')

##____________________________________________________________________________||
process.pfMetWithSignificance = process.pfMet.clone(
    calculateSignificance = cms.bool(True),
    srcJets = cms.InputTag("ak4PFJetsCHS"),
    srcJetSF = cms.string('AK4PFchs'),
    srcJetResPt = cms.string('AK4PFchs_pt'),
    srcJetResPhi = cms.string('AK4PFchs_phi'),
    srcRho = cms.InputTag('fixedGridRhoAll'),
    srcLeptons = cms.VInputTag("selectedElectrons", "selectedMuons", "selectedPhotons"),
    parameters = process.METSignificanceParams
    )

##____________________________________________________________________________||
process.p = cms.Path(
    process.selectionSequenceForMETSig *
    process.pfMetWithSignificance *
    process.pfMet
    )

process.e1 = cms.EndPath(
    process.out
    )

##____________________________________________________________________________||
