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
   input = cms.untracked.int32(-1)
)

#configurable options =======================================================================
usePrivateSQlite =True #use external JECs (sqlite file)
useHFCandidates =False #use HF for MET and Type1 computation
#===================================================================


### External JECs =====================================================================================================

from Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff import *
process.GlobalTag.globaltag = 'MCRUN2_74_v9'

if usePrivateSQlite:
    from CondCore.DBCommon.CondDBSetup_cfi import *
    era="Summer15_50nsV2_MC"
    process.jec = cms.ESSource("PoolDBESSource",CondDBSetup,
                               connect = cms.string( "sqlite_file:PhysicsTools/PatAlgos/test/"+era+".db" ),
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
process.source = cms.Source("PoolSource", 
    fileNames = cms.untracked.vstring([
            "root://eoscms//eos/cms/store/mc/RunIISpring15DR74/DYJetsToLL_M-50_TuneCUETP8M1_13TeV-amcatnloFXFX-pythia8/MINIAODSIM/Asympt25ns_MCRUN2_74_V9-v3/10000/009D49A5-7314-E511-84EF-0025905A605E.root",
    ])
)



### ---------------------------------------------------------------------------
### Removing the HF from the MET computation
### ---------------------------------------------------------------------------
pfCands="packedPFCandidates"
if not useHFCandidates:
    process.noHFCands = cms.EDFilter("CandPtrSelector",
                                     src=cms.InputTag("packedPFCandidates"),
                                     cut=cms.string("pdgId!=1 && pdgId!=2 && abs(eta)<3.0")
                                     )
    pfCands="noHFCands"

#jets are rebuilt from those candidates by the tools, no need to do anything else
### =================================================================================


from PhysicsTools.PatUtils.tools.runMETCorrectionsAndUncertainties import runMetCorAndUncFromMiniAOD

#default configuration for miniAOD reprocessing, change the isData flag to run on data
#for a full met computation, remove the pfCandColl input
runMetCorAndUncFromMiniAOD(process,
                           isData=False,
                           pfCandColl=cms.InputTag(pfCands)
                           )

### -------------------------------------------------------------------
### the lines below remove the L2L3 residual corrections when processing data
### MM 081415: new recommendations, use the L2L3 residual corrections -> commentting
### -------------------------------------------------------------------
process.patPFMetT1T2Corr.jetCorrLabelRes = cms.InputTag("L3Absolute")
process.patPFMetT1T2SmearCorr.jetCorrLabelRes = cms.InputTag("L3Absolute")
process.patPFMetT2Corr.jetCorrLabelRes = cms.InputTag("L3Absolute")
process.patPFMetT2SmearCorr.jetCorrLabelRes = cms.InputTag("L3Absolute")
process.shiftedPatJetEnDown.jetCorrLabelUpToL3Res = cms.InputTag("ak4PFCHSL1FastL2L3Corrector")
process.shiftedPatJetEnUp.jetCorrLabelUpToL3Res = cms.InputTag("ak4PFCHSL1FastL2L3Corrector")
### ------------------------------------------------------------------


process.MINIAODSIMoutput = cms.OutputModule("PoolOutputModule",
    compressionLevel = cms.untracked.int32(4),
    compressionAlgorithm = cms.untracked.string('LZMA'),
    eventAutoFlushCompressedSize = cms.untracked.int32(15728640),
    outputCommands = cms.untracked.vstring( "keep *_slimmedMETs_*_*",
                                            "keep *_patPFMetT1Txy_*_*",
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
