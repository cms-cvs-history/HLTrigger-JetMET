/** \class HLTMhtHtFilter
 *
 *
 *  \author Gheorghe Lungu
 *
 */

#include "HLTrigger/JetMET/interface/HLTMhtHtFilter.h"

#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/JetReco/interface/CaloJetCollection.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"

#include "DataFormats/Math/interface/deltaPhi.h"

//
// constructors and destructor
//
HLTMhtHtFilter::HLTMhtHtFilter(const edm::ParameterSet& iConfig)
{
   inputJetTag_ = iConfig.getParameter< edm::InputTag > ("inputJetTag");
   saveTag_     = iConfig.getUntrackedParameter<bool>("saveTag",false);
   minMht_= iConfig.getParameter<double> ("minMht"); 
   minPtJet_= iConfig.getParameter<std::vector<double> > ("minPtJet"); 
   minNJet_= iConfig.getParameter<int> ("minNJet"); 
   mode_= iConfig.getParameter<int>("mode");
   //----mode=1 for MHT only 
   //----mode=2 for Meff
   //----mode=3 for PT12
   //----mode=4 for HT only
   //----mode=5 for MHT/HT ratio
   etaJet_= iConfig.getParameter<std::vector<double> > ("etaJet");
   usePt_= iConfig.getParameter<bool>("usePt");
   minPT12_= iConfig.getParameter<double> ("minPT12");
   minMeff_= iConfig.getParameter<double> ("minMeff");
   minHt_= iConfig.getParameter<double> ("minHt");
   minMht2Ht_= iConfig.getParameter<double> ("minMht2Ht");

   //register your products
   produces<trigger::TriggerFilterObjectWithRefs>();
}

HLTMhtHtFilter::~HLTMhtHtFilter(){}


// ------------ method called to produce the data  ------------
bool
HLTMhtHtFilter::filter(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace std;
  using namespace edm;
  using namespace reco;
  using namespace trigger;
  // The filter object
  auto_ptr<trigger::TriggerFilterObjectWithRefs> filterobject (new trigger::TriggerFilterObjectWithRefs(path(),module()));
  if (saveTag_) filterobject->addCollectionTag(inputJetTag_);

  CaloJetRef ref;
  // Get the Candidates

  Handle<CaloJetCollection> recocalojets;
  iEvent.getByLabel(inputJetTag_,recocalojets);

  // look at all candidates,  check cuts and add to filter object
  int n(0), nj(0), flag(0);
  double ht=0.;
  double mhtx=0., mhty=0.;
  double jetVar, mht2ht(0.);

  if(recocalojets->size() > 0){
    // events with at least one jet
    for (CaloJetCollection::const_iterator recocalojet = recocalojets->begin(); 
	 recocalojet != recocalojets->end(); recocalojet++) {

      jetVar = recocalojet->pt();
      if (!usePt_ || mode_==3) jetVar = recocalojet->et();

      if (mode_==1 || mode_==2 || mode_==5) {//---get MHT
	if (jetVar > minPtJet_[1] && fabs(recocalojet->eta()) < etaJet_[1]) {
	  mhtx -= jetVar*cos(recocalojet->phi());
	  mhty -= jetVar*sin(recocalojet->phi());
	}
      }
      if (mode_==2 || mode_==4 || mode_==5) {//---get HT
	if (jetVar > minPtJet_[0] && fabs(recocalojet->eta()) < etaJet_[0]) {
	  ht += jetVar;
	  nj++;
	}
      }
      if (mode_==3) {//---get PT12
	if (jetVar > minPtJet_[0] && fabs(recocalojet->eta()) < etaJet_[0]) {
	  nj++;
	  mhtx -= jetVar*cos(recocalojet->phi());
	  mhty -= jetVar*sin(recocalojet->phi());
	  if (nj==2) break;
	}
      }
    }
    
    if( mode_==1 && sqrt(mhtx*mhtx + mhty*mhty) > minMht_) flag=1;
    if( mode_==2 && sqrt(mhtx*mhtx + mhty*mhty)+ht > minMeff_) flag=1;
    if( mode_==3 && sqrt(mhtx*mhtx + mhty*mhty) > minPT12_ && nj>1) flag=1;
    if( mode_==4 && ht > minHt_) flag=1;
    if( mode_==5 && nj>2) {
      if (ht>0) mht2ht = sqrt(mhtx*mhtx + mhty*mhty)/ht;
      if (mht2ht > minMht2Ht_) flag=1;
    }
    if (flag==1) {
      for (reco::CaloJetCollection::const_iterator recocalojet = recocalojets->begin(); recocalojet!=recocalojets->end(); recocalojet++) {
	jetVar = recocalojet->pt();
	if (!usePt_ || mode_==3) jetVar = recocalojet->et();

	if (jetVar > minPtJet_[0]) {
	  ref = CaloJetRef(recocalojets,distance(recocalojets->begin(),recocalojet));
	  filterobject->addObject(TriggerJet,ref);
	  n++;
	}
      }
    }
    
  } // events with at least one jet
  
  
  
  // filter decision
  bool accept(n>0);
  
  // put filter object into the Event
  iEvent.put(filterobject);
  
  return accept;
}
