/** \class EleFiller
 *
 *  No description available.
 *
 *  $Date: 2013/05/24 15:42:42 $
 *  $Revision: 1.28 $
 *  \author N. Amapane (Torino)
 *  \author S. Bolognesi (JHU)
 *  \author C. Botta (CERN)
 *  \author S. Casasso (Torino)
 */

#include <FWCore/Framework/interface/Frameworkfwd.h>
#include <FWCore/Framework/interface/EDProducer.h>
#include <FWCore/Framework/interface/Event.h>
#include <FWCore/ParameterSet/interface/ParameterSet.h>
#include <FWCore/Framework/interface/ESHandle.h>

#include <ZZAnalysis/AnalysisStep/interface/CutSet.h>
#include <ZZAnalysis/AnalysisStep/interface/LeptonIsoHelper.h>

#include "EgammaAnalysis/ElectronTools/interface/EGammaMvaEleEstimatorCSA14.h"

#include <vector>
#include <string>

using namespace edm;
using namespace std;
using namespace reco;


bool recomputeBDT = true; // true = pick BDT from VID; false = from existing userfloat

class EleFiller : public edm::EDProducer {
 public:
  /// Constructor
  explicit EleFiller(const edm::ParameterSet&);
    
  /// Destructor
  ~EleFiller(){
    //delete myMVATrig;
  };  

 private:
  virtual void beginJob(){};  
  virtual void produce(edm::Event&, const edm::EventSetup&);
  virtual void endJob(){};

  edm::EDGetTokenT<pat::ElectronRefVector> electronToken;
  int sampleType;
  int setup;
  const StringCutObjectSelector<pat::Electron, true> cut;
  const CutSet<pat::Electron> flags;
  edm::EDGetTokenT<double> rhoToken;
  edm::EDGetTokenT<vector<Vertex> > vtxToken;
  //EGammaMvaEleEstimatorCSA14* myMVATrig;
  EDGetTokenT<ValueMap<float> > BDTValueMapToken;
};


EleFiller::EleFiller(const edm::ParameterSet& iConfig) :
  electronToken(consumes<pat::ElectronRefVector>(iConfig.getParameter<edm::InputTag>("src"))),
  sampleType(iConfig.getParameter<int>("sampleType")),
  setup(iConfig.getParameter<int>("setup")),
  cut(iConfig.getParameter<std::string>("cut")),
  flags(iConfig.getParameter<ParameterSet>("flags")),
  //myMVATrig(0),
  BDTValueMapToken(consumes<ValueMap<float> >(iConfig.getParameter<InputTag>("mvaValuesMap")))
{
  rhoToken = consumes<double>(LeptonIsoHelper::getEleRhoTag(sampleType, setup));
  vtxToken = consumes<vector<Vertex> >(edm::InputTag("goodPrimaryVertices"));
  produces<pat::ElectronCollection>();

}


void
EleFiller::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{  

  // Get leptons and rho
  edm::Handle<pat::ElectronRefVector> electronHandle;
  iEvent.getByToken(electronToken, electronHandle);

  edm::Handle<double> rhoHandle;
  iEvent.getByToken(rhoToken, rhoHandle);
  double rho = *rhoHandle;

  edm::Handle<vector<Vertex> > vertices;
  iEvent.getByToken(vtxToken,vertices);

  Handle<ValueMap<float> > BDTValues;
  iEvent.getByToken(BDTValueMapToken, BDTValues);

  // Output collection
  auto result = std::make_unique<pat::ElectronCollection>();

  for (unsigned int i = 0; i< electronHandle->size(); ++i){

    //---Clone the pat::Electron
    pat::Electron l(*((*electronHandle)[i].get()));

    //--- PF ISO
    // for cone size R=0.4 :
    //float PFChargedHadIso   = l.chargedHadronIso();
    //float PFNeutralHadIso   = l.neutralHadronIso();
    //float PFPhotonIso       = l.photonIso();
    // for cone size R=0.3 :
    float PFChargedHadIso   = l.pfIsolationVariables().sumChargedHadronPt;
    float PFNeutralHadIso   = l.pfIsolationVariables().sumNeutralHadronEt;
    float PFPhotonIso       = l.pfIsolationVariables().sumPhotonEt;

    float SCeta = l.superCluster()->eta(); 
    float fSCeta = fabs(SCeta);

    float combRelIsoPF = LeptonIsoHelper::combRelIsoPF(sampleType, setup, rho, l);

    //--- SIP, dxy, dz
    float IP      = fabs(l.dB(pat::Electron::PV3D));
    float IPError = l.edB(pat::Electron::PV3D);
    float SIP     = IP/IPError;

    float dxy = 999.;
    float dz  = 999.;
    const Vertex* vertex = 0;
    if (vertices->size()>0) {
      vertex = &(vertices->front());
      dxy = fabs(l.gsfTrack()->dxy(vertex->position()));
      dz  = fabs(l.gsfTrack()->dz(vertex->position()));
    } 

    
    // RunII BDT ID
    float BDT = 0.;
    if (recomputeBDT) {

      //BDT running VID
      BDT = (*BDTValues)[(*electronHandle)[i]];

    } else {

      //Spring15 BDT taking the userfloat (possible as of MiniAODv2 of 74X)
      BDT = l.userFloat("ElectronMVAEstimatorRun2Spring15NonTrig25nsV1Values");

    }
    
    float pt = l.pt();

//     //Legacy cuts
//     bool isBDT = (pt <= 10 && (( fSCeta < 0.8 && BDT > 0.47)  ||
// 			       (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > 0.004) ||
// 			       (fSCeta >= 1.479               && BDT > 0.295))) ||
//                  //Moriond13 eID cuts updated for the paper
// 		 //(pt >  10 && ((fSCeta < 0.8 && BDT > 0.5)  ||
// 		 //	       (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > 0.12) ||
//                  (pt >  10 && ((fSCeta < 0.8 && BDT > -0.34)  ||
// 			       (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > -0.65) ||
// 			       (fSCeta >= 1.479               && BDT > 0.6)));

//    //WP for fixed Phys14-based BDT
//    bool isBDT = (pt <= 10 && ((fSCeta < 0.8                    && BDT > -0.586) ||
//                               (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > -0.712) ||
//                               (fSCeta >= 1.479                 && BDT > -0.662)   )) ||
//                 (pt >  10 && ((fSCeta < 0.8                    && BDT > -0.652) ||
//                               (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > -0.701) ||
//                               (fSCeta >= 1.479                 && BDT > -0.350)   ));

    // WP for Spring15-based BDT as proposed in https://indico.cern.ch/event/439325/session/1/contribution/21/attachments/1156760/1663207/slides_20150918.pdf
//     bool isBDT = (pt <= 10 && ((fSCeta < 0.8                    && BDT > -0.265) ||
//                                (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > -0.556) ||
//                                (fSCeta >= 1.479                 && BDT > -0.551)   )) ||
//                  (pt >  10 && ((fSCeta < 0.8                    && BDT > -0.072) ||
//                                (fSCeta >= 0.8 && fSCeta < 1.479 && BDT > -0.286) ||
//                                (fSCeta >= 1.479                 && BDT > -0.267)   ));

    //WP for preliminary 8X ID BDT
    bool isBDT = (pt<=10 && ((fSCeta<0.8                  && BDT > -0.211) ||
                                (fSCeta>=0.8 && fSCeta<1.479 && BDT > -0.396) ||
                                (fSCeta>=1.479               && BDT > -0.215))) 
                 || (pt>10  && ((fSCeta<0.8                  && BDT > -0.870) ||
                                (fSCeta>=0.8 && fSCeta<1.479 && BDT > -0.838) || 
                                (fSCeta>=1.479               && BDT > -0.763)));


    //-- Missing hit  
	 int missingHit;
	 #if CMSSW_VERSION_MAJOR < 9
	 missingHit = l.gsfTrack()->hitPattern().numberOfHits(HitPattern::MISSING_INNER_HITS);
    #else
	 missingHit = l.gsfTrack()->hitPattern().numberOfAllHits(HitPattern::MISSING_INNER_HITS);
	 #endif
    //-- Flag for crack electrons (which use different efficiency SFs)
    bool isCrack = l.isGap(); 
    //--- Trigger matching
    int HLTMatch = 0; //FIXME
    
    //--- Embed user variables
    l.addUserFloat("PFChargedHadIso",PFChargedHadIso);
    l.addUserFloat("PFNeutralHadIso",PFNeutralHadIso);
    l.addUserFloat("PFPhotonIso",PFPhotonIso);
    l.addUserFloat("combRelIsoPF",combRelIsoPF);
    l.addUserFloat("SCeta",SCeta);
    l.addUserFloat("rho",rho);
    l.addUserFloat("SIP",SIP);
    l.addUserFloat("dxy",dxy);
    l.addUserFloat("dz",dz);
    l.addUserFloat("BDT",BDT);    
    l.addUserFloat("isBDT",isBDT);
    l.addUserFloat("isCrack",isCrack);
    l.addUserFloat("HLTMatch", HLTMatch);
    l.addUserFloat("missingHit", missingHit);

    //--- MC parent code 
//     MCHistoryTools mch(iEvent);
//     if (mch.isMC()) {
//       int MCParentCode = 0;
//       //      int MCParentCode = mch.getParentCode(&l); //FIXME: does not work on cmg
//       l.addUserFloat("MCParentCode",MCParentCode);
//     }

    //--- Check selection cut. Being done here, flags are not available; but this way we 
    //    avoid wasting time on rejected leptons.
    if (!cut(l)) continue;

    //--- Embed flags (ie flags specified in the "flags" pset)
    for(CutSet<pat::Electron>::const_iterator flag = flags.begin(); flag != flags.end(); ++flag) {
      l.addUserFloat(flag->first,int((*(flag->second))(l)));
    }

    result->push_back(l);
  }
  iEvent.put(std::move(result));
}


#include <FWCore/Framework/interface/MakerMacros.h>
DEFINE_FWK_MODULE(EleFiller);

