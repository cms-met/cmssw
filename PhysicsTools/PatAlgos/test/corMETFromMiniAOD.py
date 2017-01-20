import FWCore.ParameterSet.Config as cms

# Define the CMSSW process
process = cms.Process("RERUN")

# Load the standard set of configuration modules
process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.GeometryDB_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

# Message Logger settings
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.destinations = ['cout', 'cerr']
process.MessageLogger.cerr.FwkReport.reportEvery = 1

# Set the process options -- Display summary at the end, enable unscheduled execution
process.options = cms.untracked.PSet( 
    allowUnscheduled = cms.untracked.bool(True),
    wantSummary = cms.untracked.bool(False) 
)

# How many events to process
process.maxEvents = cms.untracked.PSet( 
   input = cms.untracked.int32(100)
)

#configurable options =======================================================================
runOnData=True #data/MC switch
usePrivateSQlite=False #use external JECs (sqlite file)
useHFCandidates=True #create an additionnal NoHF slimmed MET collection if the option is set to false
redoPuppi=False # rebuild puppiMET
#===================================================================


### External JECs =====================================================================================================

#from Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff import *
#process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
#from Configuration.AlCa.GlobalTag_condDBv2 import GlobalTag
from Configuration.AlCa.autoCond import autoCond
if runOnData:
  process.GlobalTag.globaltag = autoCond['run2_data']
else:
  process.GlobalTag.globaltag = autoCond['run2_mc']

if usePrivateSQlite:
    from CondCore.DBCommon.CondDBSetup_cfi import *
    import os
    if runOnData:
      era="Summer15_25nsV6_DATA"
    else:
      era="Summer15_25nsV6_MC"
      
    process.jec = cms.ESSource("PoolDBESSource",CondDBSetup,
                               connect = cms.string( "frontier://FrontierPrep/CMS_COND_PHYSICSTOOLS"),
                               toGet =  cms.VPSet(
            cms.PSet(
                record = cms.string("JetCorrectionsRecord"),
                tag = cms.string("JetCorrectorParametersCollection_"+era+"_AK4PF"),
                label= cms.untracked.string("AK4PF")
                ),
            cms.PSet(
                record = cms.string("JetCorrectionsRecord"),
                tag = cms.string("JetCorrectorParametersCollection_"+era+"_AK4PFchs"),
                label= cms.untracked.string("AK4PFchs")
                ),
            )
                               )
    process.es_prefer_jec = cms.ESPrefer("PoolDBESSource",'jec')



### =====================================================================================================
# Define the input source
if runOnData:
  fname="/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_8.root"
  #fname = '/store/relval/CMSSW_8_0_20/MET/MINIAOD/80X_dataRun2_relval_Candidate_2016_09_02_10_27_40_RelVal_met2016B-v1/00000/2E6B9138-1C7A-E611-AE72-0025905A60DE.root' 
else:
  #fname = '/store/relval/CMSSW_8_0_20/RelValZMM_13/MINIAODSIM/80X_mcRun2_asymptotic_2016_TrancheIV_v4_Tr4GT_v4-v1/00000/64F9C946-C57A-E611-AA05-0CC47A74527A.root'
  fname="file:/tmp/mmarionn/test.root"

# Define the input source
process.source = cms.Source("PoolSource", 
    fileNames = cms.untracked.vstring( #[ fname ])
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_1.root"
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_2.root",
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_3.root",
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_4.root",
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_5.root",
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_6.root",
    #"/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_7.root",
    "/store/user/zdemirag/metscan/DoubleMuon/crab_pickEventsRereco/161220_090009/0000/pickevents_rereco_8.root"
    )
)


### ---------------------------------------------------------------------------
### Removing the HF from the MET computation
### ---------------------------------------------------------------------------
if not useHFCandidates:
    process.noHFCands = cms.EDFilter("CandPtrSelector",
                                     src=cms.InputTag("packedPFCandidates"),
                                     cut=cms.string("abs(pdgId)!=1 && abs(pdgId)!=2 && abs(eta)<3.0")
                                     )

#jets are rebuilt from those candidates by the tools, no need to do anything else
### =================================================================================

from PhysicsTools.PatUtils.tools.runMETCorrectionsAndUncertainties import runMetCorAndUncFromMiniAOD

#default configuration for miniAOD reprocessing, change the isData flag to run on data
#for a full met computation, remove the pfCandColl input
runMetCorAndUncFromMiniAOD(process,
                           isData=runOnData,
                           )

if not useHFCandidates:
    runMetCorAndUncFromMiniAOD(process,
                               isData=runOnData,
                               pfCandColl=cms.InputTag("noHFCands"),
                               reclusterJets=True, #needed for NoHF
                               recoMetFromPFCs=True, #needed for NoHF
                               postfix="NoHF"
                               )

if redoPuppi:
  from PhysicsTools.PatAlgos.slimming.puppiForMET_cff import makePuppiesFromMiniAOD
  makePuppiesFromMiniAOD( process );

  runMetCorAndUncFromMiniAOD(process,
                             isData=runOnData,
                             metType="Puppi",
                             pfCandColl=cms.InputTag("puppiForMET"),
                             recoMetFromPFCs=True,
                             reclusterJets=True,
                             jetFlavor="AK4PFPuppi",
                             postfix="Puppi"
                             )


from PhysicsTools.PatUtils.tools.muonRecoMitigation import muonRecoMitigation
muonRecoMitigation(process,
                   pfCandCollection="packedPFCandidates",
                   #muonCollection="badGlobalMuonTagger", #cloneGlobalMuonTagger
                   runOnMiniAOD=True,
                   cleaningScheme="computeAllApplyClone"
                   )

runMetCorAndUncFromMiniAOD(process,
                          isData=runOnData,
                           pfCandColl="cleanMuonsPFCandidates",
                           recoMetFromPFCs=True,
                           postfix="MuClean"
                           )

process.MINIAODSIMoutput = cms.OutputModule("PoolOutputModule",
    compressionLevel = cms.untracked.int32(4),
    compressionAlgorithm = cms.untracked.string('LZMA'),
    eventAutoFlushCompressedSize = cms.untracked.int32(15728640),
    outputCommands = cms.untracked.vstring( "keep *_slimmedMETs_*_*",
                                            "keep *_slimmedMETsMuClean_*_*",
                                            "keep *_slimmedMETsNoHF_*_*",
                                            "keep *_patPFMet_*_*",
                                            "keep *_patPFMetT1_*_*",
                                            "keep *_patPFMetT1JetResDown_*_*",
                                            "keep *_patPFMetT1JetResUp_*_*",
                                            "keep *_patPFMetT1Smear_*_*",
                                            "keep *_patPFMetT1SmearJetResDown_*_*",
                                            "keep *_patPFMetT1SmearJetResUp_*_*",
                                            "keep *_patPFMetT1Puppi_*_*",
                                            "keep *_slimmedMETsPuppi_*_*",
                                            "keep *_corEGSlimmedMET_*_*",
                                            "keep *_slimmedPhotons_*_*",
                                            "keep *_slimmedElectrons_*_*",
                                            "keep *_cleanedPhotons_*_*",
                                            "keep *_cleanedCorPhotons_*_*",
                                            "keep *_corMETPhoton_*_*",
                                            "keep *_corMETElectron_*_*",
                                            "keep *_badGlobalMuonTagger_*_*",
                                            "keep *_cloneGlobalMuonTagger_*_*",
                                            "keep *_cleanMuonsPFCandidates_*_*",
                                            "keep *_superbadMuons_*_*",
                                            "keep *_cleanMuonsPFCandidates_*_*",
                                            "keep *_packedPFCandidates_*_*"
                                            
                                            ),
    fileName = cms.untracked.string('corMETMiniAOD.root'),
    dataset = cms.untracked.PSet(
        filterName = cms.untracked.string(''),
        dataTier = cms.untracked.string('')
    ),
    dropMetaData = cms.untracked.string('ALL'),
    fastCloning = cms.untracked.bool(False),
    overrideInputFileSplitLevels = cms.untracked.bool(True)
)


process.MINIAODSIMoutput_step = cms.EndPath(process.MINIAODSIMoutput)
