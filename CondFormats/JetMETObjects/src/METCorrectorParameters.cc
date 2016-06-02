// Generic parameters for MET corrections
//
#include "CondFormats/JetMETObjects/interface/METCorrectorParameters.h"
#include "CondFormats/JetMETObjects/interface/Utilities.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <iterator>
//#include <string>

//------------------------------------------------------------------------ 
//--- METCorrectorParameters::Definitions constructor --------------------
//--- takes specific arguments for the member variables ------------------
//------------------------------------------------------------------------
METCorrectorParameters::Definitions::Definitions(const std::vector<std::string>& fBinVar, const std::vector<int>& fParVar, const std::string& fFormula )
{
  for(unsigned i=0;i<fBinVar.size();i++)
    mBinVar.push_back(fBinVar[i]);
  for(unsigned i=0;i<fParVar.size();i++)
    mParVar.push_back(fParVar[i]);
  mFormula      = fFormula;
}
//------------------------------------------------------------------------
//--- METCorrectorParameters::Definitions constructor --------------------
//--- reads the member variables from a string ---------------------------
//------------------------------------------------------------------------
METCorrectorParameters::Definitions::Definitions(const std::string& fLine)
{
  std::vector<std::string> tokens = getTokens(fLine);
  // corrType N_bin binVa.. var formula
  if (!tokens.empty())
  { 
    if (tokens.size() < 6) 
    {
      std::stringstream sserr;
      sserr<<"(line "<<fLine<<"): Great than or equal to 6 expected tokens:"<<tokens.size();
      handleError("METCorrectorParameters::Definitions",sserr.str());
    }
    // No. of Bin Variable
    //std::cout<<"Definitions==========="<<std::endl;
    //mdataType = tokens[0]; 
    //std::cout<<"dataType:"<<ptclType_<<"\t";
    ptclType_ = getSigned(tokens[0]);
    //std::cout<<"ptclType:"<<ptclType_<<"\t";
    unsigned nBinVar = getUnsigned(tokens[1]);
    //std::cout<<"nBinVar:"<<tokens[1]<<"\t";
    for(unsigned i=0;i<nBinVar;i++)
    {
      mBinVar.push_back(tokens[i+2]);
      //std::cout<<tokens[i+2]<<"\t";
    }
    // Num.o of Parameterization Variable
    unsigned nParVar = getUnsigned(tokens[nBinVar+2]);
    //std::cout<<"nParVar: "<<tokens[nBinVar+2]<<"\t";
    for(unsigned i=0;i<nParVar;i++)
    {
      mParVar.push_back(getSigned(tokens[nBinVar+3+i]));
      //std::cout<<tokens[nBinVar+3+i]<<"\t";
    }
    mFormula = tokens[nParVar+nBinVar+3];
    //std::cout<<"Formula: "<<tokens[nParVar+nBinVar+3]<<std::endl;
    if (tokens.size() != nParVar+nBinVar+4 ) 
    {
      std::stringstream sserr;
      sserr<<"(line "<<fLine<<"): token size should be:"<<nParVar+nBinVar+3<<" but it is "<<tokens.size();
      handleError("METCorrectorParameters::Definitions",sserr.str());
    }
  }
}
//------------------------------------------------------------------------
//--- METCorrectorParameters::Record constructor -------------------------
//--- reads the member variables from a string ---------------------------
//------------------------------------------------------------------------
METCorrectorParameters::Record::Record(const std::string& fLine,unsigned fNvar) : mMin(0), mMax(0), mParameters(0)
{
  mNvar = fNvar;
  // quckly parse the line
  std::vector<std::string> tokens = getTokens(fLine);
  if (!tokens.empty())
  { 
    if (tokens.size() < 5) 
    {
      std::stringstream sserr;
      sserr<<"(line "<<fLine<<"): "<<"three tokens expected, "<<tokens.size()<<" provided.";
      handleError("METCorrectorParameters::Record",sserr.str());
    }
    //std::cout<<"Record ==============="<<std::endl;
    mMetAxis = tokens[0];
    //std::cout<<mMetAxis<<"\t";
    for(unsigned i=0;i<mNvar;i++)
    {
      mMin.push_back(getFloat(tokens[i*2+1]));
      mMax.push_back(getFloat(tokens[i*2+2]));
      //std::cout<<tokens[i*2+1]<<"\t";
      //std::cout<<tokens[i*2+2]<<"\t";
    }
    unsigned nParam = getUnsigned(tokens[2*mNvar+1]);
    //std::cout<<tokens[2*mNvar+1]<<"\t";
    if (nParam != tokens.size()-(2*mNvar+2)) 
    {
      std::stringstream sserr;
      sserr<<"(line "<<fLine<<"): "<<tokens.size()-(2*mNvar+1)<<" parameters, but nParam="<<nParam<<".";
      handleError("METCorrectorParameters::Record",sserr.str());
    }
    for (unsigned i = (2*mNvar+2); i < tokens.size(); ++i)
    {
      mParameters.push_back(getFloat(tokens[i]));
      //std::cout<<tokens[i]<<std::endl;
    }
  }
}
//------------------------------------------------------------------------
//--- METCorrectorParameters constructor ---------------------------------
//--- reads the member variables from a string ---------------------------
//------------------------------------------------------------------------
METCorrectorParameters::METCorrectorParameters(const std::string& fFile, const std::string& fSection) 
{ 
  std::ifstream input(fFile.c_str());
  std::string currentSection = "";
  std::string line;
  std::string currentDefinitions = "";
  while (std::getline(input,line)) 
  {
    //std::cout << " Line of parameters " << line << std::endl;
    std::string section = getSection(line);
    std::string tmp = getDefinitions(line);
    if (!section.empty() && tmp.empty()) 
    {
      currentSection = section;
      continue;
    }
    if (currentSection == fSection) 
    {
      if (!tmp.empty()) 
      {
        currentDefinitions = tmp;
        continue; 
      }
      Definitions definitions(currentDefinitions);
      if (!(definitions.nBinVar()==0 && definitions.formula()==""))
        mDefinitions = definitions;
      Record record(line,mDefinitions.nBinVar());
      bool check(true);
      for(unsigned i=0;i<mDefinitions.nBinVar();++i)
        if (record.xMin(i)==0 && record.xMax(i)==0)
          check = false;
      if (record.nParameters() == 0)
        check = false;  
      if (check)
        mRecords.push_back(record);
    } 
  }
  if (currentDefinitions=="")
    handleError("METCorrectorParameters","No definitions found!!!");
  if (mRecords.empty() && currentSection == "") mRecords.push_back(Record());
  if (mRecords.empty() && currentSection != "") 
  {
    std::stringstream sserr; 
    sserr<<"the requested section "<<fSection<<" doesn't exist!";
    handleError("METCorrectorParameters",sserr.str()); 
  }
  std::sort(mRecords.begin(), mRecords.end());
  valid_ = true;
}
//------------------------------------------------------------------------
//--- returns the index of the record defined by fX ----------------------
//------------------------------------------------------------------------
/*
int METCorrectorParameters::binIndex(const std::vector<float>& fX) const 
{
  int result = -1;
  unsigned N = mDefinitions.nVar();
  if (N != fX.size()) 
    {
      std::stringstream sserr; 
      sserr<<"# bin variables "<<N<<" doesn't correspont to requested #: "<<fX.size();
      handleError("METCorrectorParameters",sserr.str());
    }
  unsigned tmp;
  for (unsigned i = 0; i < size(); ++i) 
    {
      tmp = 0;
      for (unsigned j=0;j<N;j++)
        if (fX[j] >= record(i).xMin(j) && fX[j] < record(i).xMax(j))
          tmp+=1;
      if (tmp==N)
        { 
          result = i;
          break;
        }
    } 
  return result;
}
*/
//------------------------------------------------------------------------
//--- returns the neighbouring bins of fIndex in the direction of fVar ---
//------------------------------------------------------------------------
/*
int METCorrectorParameters::neighbourBin(unsigned fIndex, unsigned fVar, bool fNext) const 
{
  int result = -1;
  unsigned N = mDefinitions.nVar();
  if (fVar >= N) 
    {
      std::stringstream sserr; 
      sserr<<"# of bin variables "<<N<<" doesn't correspond to requested #: "<<fVar;
      handleError("METCorrectorParameters",sserr.str()); 
    }
  unsigned tmp;
  for (unsigned i = 0; i < size(); ++i) 
    {
      tmp = 0;
      for (unsigned j=0;j<fVar;j++)
        if (fabs(record(i).xMin(j)-record(fIndex).xMin(j))<0.0001)
          tmp+=1;
      for (unsigned j=fVar+1;j<N;j++)
        if (fabs(record(i).xMin(j)-record(fIndex).xMin(j))<0.0001)
          tmp+=1;
      if (tmp<N-1)
        continue; 
      if (tmp==N-1)
        {
          if (fNext)
            if (fabs(record(i).xMin(fVar)-record(fIndex).xMax(fVar))<0.0001)
              tmp+=1;
          if (!fNext)
            if (fabs(record(i).xMax(fVar)-record(fIndex).xMin(fVar))<0.0001)
              tmp+=1;
        } 
      if (tmp==N)
        { 
          result = i;
          break;
        }
    } 
  return result;
}
*/
//------------------------------------------------------------------------
//--- returns the number of bins in the direction of fVar ----------------
//------------------------------------------------------------------------
/*
unsigned METCorrectorParameters::size(unsigned fVar) const
{
  if (fVar >= mDefinitions.nVar()) 
    { 
      std::stringstream sserr; 
      sserr<<"requested bin variable index "<<fVar<<" is greater than number of variables "<<mDefinitions.nVar();
      handleError("METCorrectorParameters",sserr.str()); 
    }    
  unsigned result = 0;
  float tmpMin(-9999),tmpMax(-9999);
  for (unsigned i = 0; i < size(); ++i)
    if (record(i).xMin(fVar) > tmpMin && record(i).xMax(fVar) > tmpMax)
      { 
        result++;
        tmpMin = record(i).xMin(fVar);
        tmpMax = record(i).xMax(fVar);
      }
  return result; 
}
*/
//------------------------------------------------------------------------
//--- returns the vector of bin centers of fVar --------------------------
//------------------------------------------------------------------------
/*
std::vector<float> METCorrectorParameters::binCenters(unsigned fVar) const 
{
  std::vector<float> result;
  for (unsigned i = 0; i < size(); ++i)
    result.push_back(record(i).xMiddle(fVar));
  return result;
}
*/
//------------------------------------------------------------------------
//--- prints parameters on screen ----------------------------------------
//------------------------------------------------------------------------
void METCorrectorParameters::printScreen(const std::string &Section) const
{
  std::cout<<"--------------------------------------------"<<std::endl;
  std::cout<<"////////  PARAMETERS: //////////////////////"<<std::endl;
  std::cout<<"--------------------------------------------"<<std::endl;
  std::cout<<"["<<Section<<"]"<<"\n";
  //std::cout<<"SampleType:   "<<definitions().dataType()<<std::endl;
  std::cout<<"Number of binning variables:   "<<definitions().nBinVar()<<std::endl;
  std::cout<<"Names of binning variables:    ";
  for(unsigned i=0;i<definitions().nBinVar();i++)
    std::cout<<definitions().binVar(i)<<" ";
  std::cout<<std::endl;
  std::cout<<"--------------------------------------------"<<std::endl;
  std::cout<<"Number of parameter variables: "<<definitions().nParVar()<<std::endl;
  std::cout<<"Names of parameter variables:  ";
  for(unsigned i=0;i<definitions().nParVar();i++)
    std::cout<<definitions().parVar(i)<<" ";
  std::cout<<std::endl;
  std::cout<<"--------------------------------------------"<<std::endl;
  std::cout<<"Parametrization Formula:       "<<definitions().formula()<<std::endl;
  std::cout<<"--------------------------------------------"<<std::endl;
  std::cout<<"------- Bin contents -----------------------"<<std::endl;
  for(unsigned i=0;i<size();i++) //mRecords size
  {
    std::cout<<record(i).MetAxis()<<"  ";
    std::cout<<"nBinVar ("<<definitions().nBinVar()<<")  ";
    for(unsigned j=0;j<definitions().nBinVar();j++)
      std::cout<<record(i).xMin(j)<<" "<<record(i).xMax(j)<<" ";
    std::cout<<"nParameters ("<<record(i).nParameters()<<") ";
    for(unsigned j=0;j<record(i).nParameters();j++)
      std::cout<<record(i).parameter(j)<<" ";
    std::cout<<std::endl;
  }
}
//------------------------------------------------------------------------
//--- prints parameters on file ----------------------------------------
//------------------------------------------------------------------------
void METCorrectorParameters::printFile(const std::string& fFileName,const std::string &Section) const
{
  std::ofstream txtFile;
  txtFile.open(fFileName.c_str(),std::ofstream::app);
  txtFile.setf(std::ios::right);
  txtFile<<"["<<Section<<"]"<<"\n";
  txtFile<<"{"<<" "<<definitions().ptclType()<<"  "<<definitions().nBinVar();
  for(unsigned i=0;i<definitions().nBinVar();i++)
    txtFile<<"  "<<definitions().binVar(i);
  txtFile<<"  "<<definitions().nParVar();
  for(unsigned i=0;i<definitions().nParVar();i++)
    txtFile<<"  "<<definitions().parVar(i);
  txtFile<<"  "<<definitions().formula();
  //txtFile<<std::setw(definitions().formula().size()+15)
  //<<definitions().formula()<<std::setw(15);
  txtFile<<"}"<<"\n";
  for(unsigned i=0;i<size();i++) //mRecords size
  {
    txtFile<<record(i).MetAxis();
    for(unsigned j=0;j<definitions().nBinVar();j++)
      txtFile<<"  "<<record(i).xMin(j)<<"  "<<record(i).xMax(j);
    txtFile<<"  "<<record(i).nParameters();
    for(unsigned j=0;j<record(i).nParameters();j++)
      txtFile<<"  "<<record(i).parameter(j);
    txtFile<<"\n";
  }
  txtFile.close();
}

namespace {
const std::vector<std::string> labels_ = {
  "XYshiftMC",
  "XYshiftDY",
  "XYshiftTTJets",
  "XYshiftWJets",
  "XYshiftData"
};
const std::vector<std::string> XYshiftFlavors_ = {
  "hEtaPlus",
  "hEtaMinus",
  "h0Barrel",
  "h0EndcapPlus",
  "h0EndcapMinus",
  "gammaBarrel",
  "gammaEndcapPlus",
  "gammaEndcapMinus",
  "hHFPlus",
  "hHFMinus",
  "egammaHFPlus",
  "egammaHFMinus"
};

}//namespace

std::string
METCorrectorParametersCollection::findLabel( key_type k ){
  //std::cout<<"findLabel with key: "<<k<<std::endl;
  if( isXYshiftMC(k) ){
    //std::cout<<"is XYshift"<<std::endl;
    return findXYshiftMCflavor(k);
  }else if( isXYshiftDY(k) ){
    return findXYshiftDYflavor(k);
  }else if( isXYshiftTTJets(k) ){
    return findXYshiftTTJetsFlavor(k);
  }else if( isXYshiftWJets(k) ){
    return findXYshiftWJetsFlavor(k);
  }else if( isXYshiftData(k) ){
    return findXYshiftDataFlavor(k);
  }

  return labels_[k];
}
std::string
METCorrectorParametersCollection::levelName( key_type k ){
  //std::cout<<"findLabel with key: "<<k<<std::endl;
  if( isXYshiftMC(k) ){
    return labels_[XYshiftMC];
  }else if( isXYshiftDY(k) ){
    return labels_[XYshiftDY];
  }else if( isXYshiftTTJets(k) ){
    return labels_[XYshiftTTJets];
  }else if( isXYshiftWJets(k) ){
    return labels_[XYshiftWJets];
  }else if( isXYshiftData(k) ){
    return labels_[XYshiftData];
  }else{ return "Can't find the level name !!!!";}

}

std::string
METCorrectorParametersCollection::findXYshiftMCflavor( key_type k)
{
  if( k == XYshiftMC) return labels_[XYshiftMC];
  else
    return XYshiftFlavors_[k - (XYshiftMC+1)*100 -1];
}
std::string
METCorrectorParametersCollection::findXYshiftDYflavor( key_type k)
{
  if( k == XYshiftDY) return labels_[XYshiftDY];
  else
    return XYshiftFlavors_[k - (XYshiftDY+1)*100 -1];
}
std::string
METCorrectorParametersCollection::findXYshiftTTJetsFlavor( key_type k)
{
  if( k == XYshiftTTJets) return labels_[XYshiftTTJets];
  else
    return XYshiftFlavors_[k - (XYshiftTTJets+1)*100 -1];
}
std::string
METCorrectorParametersCollection::findXYshiftWJetsFlavor( key_type k)
{
  if( k == XYshiftWJets) return labels_[XYshiftWJets];
  else
    return XYshiftFlavors_[k - (XYshiftWJets+1)*100 -1];
}
std::string
METCorrectorParametersCollection::findXYshiftDataFlavor( key_type k)
{
  if( k == XYshiftData) return labels_[XYshiftData];
  else
    return XYshiftFlavors_[k - (XYshiftData+1)*100 -1];
}

void METCorrectorParametersCollection::getSections( std::string inputFile,
						    std::vector<std::string> & outputs )
{
  outputs.clear();
  std::ifstream input( inputFile.c_str() );
  while( !input.eof() ) {
    char buff[10000];
    input.getline(buff,10000);
    std::string in(buff);
    if ( in[0] == '[' ) {
      std::string tok = getSection(in);
      if ( tok != "" ) {
	outputs.push_back( tok );
      }
    }
  }
  copy(outputs.begin(),outputs.end(), std::ostream_iterator<std::string>(std::cout, "\n") );
}

// Add a METCorrectorParameter object. 
void METCorrectorParametersCollection::push_back( key_type i, value_type const & j, label_type const &flav )
{ 
  //std::cout << "Level index    = " << i << std::endl;  
  //std::cout << "flav = " << flav << std::endl;
  if( isXYshiftMC(i))
  {
    //std::cout << "This is XYshiftMC, getXYshiftFlavBin = " << getXYshiftMcFlavBin(flav) << std::endl;
    correctionsXYshift_.push_back( pair_type(getXYshiftMcFlavBin(flav),j) );
  }else if( isXYshiftDY(i))
  {
    //std::cout << "This is XYshiftDY, getXYshiftFlavBin = " << getXYshiftDyFlavBin(flav) << std::endl;
    correctionsXYshift_.push_back( pair_type(getXYshiftDyFlavBin(flav),j) );
  }else if( isXYshiftTTJets(i))
  {
    //std::cout << "This is XYshiftTTJets, getXYshiftFlavBin = " << getXYshiftTTJetsFlavBin(flav) << std::endl;
    correctionsXYshift_.push_back( pair_type(getXYshiftTTJetsFlavBin(flav),j) );
  }else if( isXYshiftWJets(i))
  {
    //std::cout << "This is XYshiftWJets, getXYshiftFlavBin = " << getXYshiftWJetsFlavBin(flav) << std::endl;
    correctionsXYshift_.push_back( pair_type(getXYshiftWJetsFlavBin(flav),j) );
  }else if( isXYshiftData(i))
  {
    //std::cout << "This is XYshiftData, getXYshiftFlavBin = " << getXYshiftDataFlavBin(flav) << std::endl;
    correctionsXYshift_.push_back( pair_type(getXYshiftDataFlavBin(flav),j) );
  }else{
    //std::cout << "***** NOT ADDING " << flav << ", corresponding position in METCorrectorParameters is not found." << std::endl;
  }
}

// Access the METCorrectorParameter via the key k.
// key_type is hashed to deal with the three collections
METCorrectorParameters const & METCorrectorParametersCollection::operator[]( key_type k ) const {
  collection_type::const_iterator ibegin, iend, i;
  if ( isXYshiftMC(k) || isXYshiftDY(k) || isXYshiftTTJets(k) || isXYshiftWJets(k) || isXYshiftData(k) ) {
    ibegin = correctionsXYshift_.begin();
    iend = correctionsXYshift_.end();
    i = ibegin;
  }
  for ( ; i != iend; ++i ) {
    if ( k == i->first ) return i->second;
  }
  throw cms::Exception("InvalidInput") << " cannot find key " << static_cast<int>(k)
				       << " in the METC payload, this usually means you have to change the global tag" << std::endl;
}

// Get a list of valid keys. These will contain hashed keys
// that are aware of all three collections.
void METCorrectorParametersCollection::validKeys(std::vector<key_type> & keys ) const {
  keys.clear();
  for ( collection_type::const_iterator ibegin = correctionsXYshift_.begin(),
	  iend = correctionsXYshift_.end(), i = ibegin; i != iend; ++i ) {
    keys.push_back( i->first );
  }
}
//void METCorrectorParametersCollection::validSections(std::vector<section_type> & sections ) const {
//  sections.clear();
//  for ( collection_type::const_iterator ibegin = correctionsXYshift_.begin(),
//	  iend = correctionsXYshift_.end(), i = ibegin; i != iend; ++i ) {
//    sections.push_back( i->first );
//  }
//}


METCorrectorParametersCollection::key_type
METCorrectorParametersCollection::getXYshiftMcFlavBin( std::string const & flav ){
  std::vector<std::string>::const_iterator found =
    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), flav );
  if ( found != XYshiftFlavors_.end() ) {
    return (found - XYshiftFlavors_.begin() + 1)+ (XYshiftMC+1) * 100;
  }
  else{
    throw cms::Exception("InvalidInput") <<
    "************** Can't find XYshiftSection: "<<flav<<std::endl;
  }
  return 0;
}
METCorrectorParametersCollection::key_type
METCorrectorParametersCollection::getXYshiftDyFlavBin( std::string const & flav ){
  std::vector<std::string>::const_iterator found =
    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), flav );
  if ( found != XYshiftFlavors_.end() ) {
    return (found - XYshiftFlavors_.begin() + 1)+ (XYshiftDY+1) * 100;
  }
  else{
    throw cms::Exception("InvalidInput") <<
    "************** Can't find XYshiftSection: "<<flav<<std::endl;
  }
  return 0;
}
METCorrectorParametersCollection::key_type
METCorrectorParametersCollection::getXYshiftTTJetsFlavBin( std::string const & flav ){
  std::vector<std::string>::const_iterator found =
    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), flav );
  if ( found != XYshiftFlavors_.end() ) {
    return (found - XYshiftFlavors_.begin() + 1)+ (XYshiftTTJets+1) * 100;
  }
  else{
    throw cms::Exception("InvalidInput") <<
    "************** Can't find XYshiftSection: "<<flav<<std::endl;
  }
  return 0;
}
METCorrectorParametersCollection::key_type
METCorrectorParametersCollection::getXYshiftWJetsFlavBin( std::string const & flav ){
  std::vector<std::string>::const_iterator found =
    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), flav );
  if ( found != XYshiftFlavors_.end() ) {
    return (found - XYshiftFlavors_.begin() + 1)+ (XYshiftWJets+1) * 100;
  }
  else{
    throw cms::Exception("InvalidInput") <<
    "************** Can't find XYshiftSection: "<<flav<<std::endl;
  }
  return 0;
}
METCorrectorParametersCollection::key_type
METCorrectorParametersCollection::getXYshiftDataFlavBin( std::string const & flav ){
  std::vector<std::string>::const_iterator found =
    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), flav );
  if ( found != XYshiftFlavors_.end() ) {
    return (found - XYshiftFlavors_.begin() + 1)+ (XYshiftData+1) * 100;
  }
  else{
    throw cms::Exception("InvalidInput") <<
    "************** Can't find XYshiftSection: "<<flav<<std::endl;
  }
  return 0;
}

bool METCorrectorParametersCollection::isXYshiftMC( key_type k ) {
  return k == XYshiftMC ||
    (k > (XYshiftMC+1)*100 && k < (XYshiftMC + 2)*100 );
}
bool METCorrectorParametersCollection::isXYshiftDY( key_type k ) {
  return k == XYshiftDY ||
    (k > (XYshiftDY+1)*100 && k < (XYshiftDY + 2)*100 );
}
bool METCorrectorParametersCollection::isXYshiftTTJets( key_type k ) {
  return k == XYshiftTTJets ||
    (k > (XYshiftTTJets+1)*100 && k < (XYshiftTTJets + 2)*100 );
}
bool METCorrectorParametersCollection::isXYshiftWJets( key_type k ) {
  return k == XYshiftWJets ||
    (k > (XYshiftWJets+1)*100 && k < (XYshiftWJets + 2)*100 );
}
bool METCorrectorParametersCollection::isXYshiftData( key_type k ) {
  return k == XYshiftData ||
    (k > (XYshiftData+1)*100 && k < (XYshiftData + 2)*100 );
}

// Find the key corresponding to each label
//METCorrectorParametersCollection::key_type
//METCorrectorParametersCollection::findKey( std::string const & label ) const {
//
//  // First check L5 corrections
//  std::vector<std::string>::const_iterator found1 =
//    find( XYshiftFlavors_.begin(), XYshiftFlavors_.end(), label );
//  if ( found1 != XYshiftFlavors_.end() ) {
//    return getXYshiftFlavBin(label);
//  }
//
//  // Finally check the default corrections
//  std::vector<std::string>::const_iterator found2 =
//    find( labels_.begin(), labels_.end(), label );
//  if ( found2 != labels_.end() ) {
//    return static_cast<key_type>(found2 - labels_.begin());
//  }
//
//  // Didn't find default corrections, throw exception
//  throw cms::Exception("InvalidInput") << " Cannot find label " << label << std::endl;
//
//}



#include "FWCore/Utilities/interface/typelookup.h"
 
TYPELOOKUP_DATA_REG(METCorrectorParameters);
TYPELOOKUP_DATA_REG(METCorrectorParametersCollection);
