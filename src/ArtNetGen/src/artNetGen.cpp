#include "artnetgen/artNetGen.h"
#include "opendb/db.h"
//#include "opendb/dbSet.h"

#include "sta/Graph.hh"
#include "sta/Sta.hh"
#include "sta/Network.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "sta/PortDirection.hh"
#include "sta/Corner.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathEnd.hh"
#include "sta/PathRef.hh"
#include "sta/Search.hh"
#include "sta/Bfs.hh"

#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "logger.h"
#include "netlist.h"
#include "node.h"


#include <fstream>
#include <sstream>
#include <math.h>
#include <unordered_map>


// for easier coding with boost

// save point and dbInst* pointer 

using std::max;
using std::min;
using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::set;
using namespace odb;
using std::stringstream;
using std::ifstream;
using std::ofstream;
using std::sqrt;
using std::unordered_map;
using std::to_string;



namespace artnetgen {

MasterInfo::MasterInfo(
    odb::dbMaster* master, 
    double ratio) : 
  master_(master),
  ratio_(ratio) {}

MasterInfo::~MasterInfo() {
  master_ = nullptr;
  ratio_ = 0; 
}

void 
MasterInfo::print() {
  std::cout << "  " << master_->getConstName() 
    << " " << ratio_ << std::endl;
}

/*
DistributionInfo::DistributionInfo(
    int value, int count, double ratio) :
  value_(value),
  count_(count),
  ratio_(ratio) {}

DistributionInfo::~DistributionInfo() {
  value_ = count_ = ratio_ = 0;
}

void
DistributionInfo::print() {
  std::cout << "  " << value_ 
    << " " << count_ 
    << " " << ratio_
    << std::endl;
}
*/

ArtNetGen::ArtNetGen() : 
    db_(nullptr), sta_(nullptr),
    instanceCnt_(5000),
    inputPinCnt_(20),
    outputPinCnt_(20),
    avgTopoOrder_(10.0), 
    avgGateDelay_(0.1),
    //binSqrt_(10),
    //maxChainDepth_(10),
    combRatio_(0.9),
    verbose_(1) {
    specFile_ = "";
    outFile_ = "";
    topModule_ = "artnet";    
} 

ArtNetGen::~ArtNetGen() {
  clear(); 
}

void
ArtNetGen::clear() {
  db_ = nullptr; 
  sta_ = nullptr;
  instanceCnt_ = 5000;
  inputPinCnt_ = 20;
  outputPinCnt_ = 20;
  //binSqrt_ = 10;
  //maxChainDepth_ = 10;
    avgTopoOrder_ = 10.0;
    avgGateDelay_ = 0.1;
  combRatio_ = 0.9;
  verbose_ = 1;
  specFile_ = "";
  outFile_ = "";
    topModule_ = "artnet";


  masters_.clear();
  masters_.shrink_to_fit();

  fiDist_.clear();
  foDist_.clear();
  bboxDist_.clear();
  edgeDist_.clear();
  //fanIns_.clear();
  //fanIns_.shrink_to_fit();
  //fanOuts_.clear();
  //fanOuts_.shrink_to_fit();
  //windows_.clear();
  //windows_.shrink_to_fit();
  //netBBoxes_.clear();
  //netBBoxes_.shrink_to_fit();
}

void 
ArtNetGen::init() {  
  if( !log_ ) {
    log_ = std::make_shared<Logger>("ANG", verbose_);
  }
}



void
ArtNetGen::writeVerilog(const char* fileName) {
    
    log_->procBegin("Write Verilog",1);

    ofstream verilog(fileName);

    if(!verilog.good()) {
        cout << "Cannot open " << fileName << " file" << endl;
        return;
    }


    verilog << "module " << topModule_ << " (" << endl;

    // 1. I/O
    vector<Node*> primIns = artnet_->getPrimaryInputs();
    vector<Node*> primOuts = artnet_->getPrimaryOutputs();
    vector<Net*> nets = artnet_->getNetlist();
    vector<Node*> nodes = artnet_->getNodes();

    for(int i=0; i < primIns.size(); i++) {
        Node* inNode = primIns[i];
        verilog << "\tinput " << inNode->getName() << "," << endl;
    }

    for(int i=0; i < primOuts.size(); i++) {
        Node* outNode = primOuts[i];
        verilog << "\toutput " << outNode->getName();

        if(i == primOuts.size()-1) {
            verilog << endl << ");" << endl;
        } else {
            verilog << "," << endl;
        }
    }

    verilog << endl << endl;

    unordered_map<Node*, unordered_map<string,string>> term2net;
    
    // 2. Wires
    for(int i=0; i < nets.size(); i++) {
        Net* net = nets[i];
        verilog << "wire " << net->getName() << ";" << endl; 

        /*
        vector<pair<Node*, string>> terms = net->getTerms();
        Node* srcNode = terms.begin()->first;
        switch(srcNode->getType()) {
            case NodeType::Combinational:
                verilog << "wire " << net->getName() << ";" << endl; break;
            case NodeType::Sequential:
                verilog << "wire " << net->getName() << ";" << endl; break;
            default:
                break;
        }
        */
        for(auto p : net->getTerms()) {
            Node* node = p.first;
            string termName = p.second;
            
            if(termName == "PIN") 
            //if(node->getType() == NodeType::PrimaryOut)
                term2net[node][termName] = node->getName();
            else
                term2net[node][termName] = net->getName();
        }
    }
  
    // 3. instances
    for(int i=0; i < nodes.size(); i++) {
        Node* node = nodes[i];
        vector<Node*> sinks = node->getSinks();
        vector<Node*> sources = node->getSources();

        if(node->getType() != NodeType::Combinational && node->getType() != NodeType::Sequential)
            continue;

        verilog << node->getDbMaster()->getName() << " " << node->getName() << "(" << endl;

        set<string> terms;

        for(int j=0; j < sources.size(); j++) {
            Node* source = sources[j];
            string termName = node->getTerm(source);
            string netName = term2net[node][termName];
            
            if(terms.find(termName) == terms.end()) {
                terms.insert(termName); 
                if(j!=0)
                    verilog << "," << endl;
                verilog << "." << termName << "(" << netName << ")";
            }
        }

        
        
        //set<string> outTerms;


        for(int j=0; j < sinks.size(); j++) {
        
            
            Node* sink = sinks[j];
            string termName = node->getTerm(sink);
            string netName = term2net[node][termName];
            
            if(terms.find(termName) == terms.end()) {
                terms.insert(termName); 
            //if(outTerms.find(termName) == outTerms.end()) {
            //    outTerms.insert(termName);
                verilog << "," << endl;
                verilog << "." << termName << "(" << netName << ")";
            }
        }

        verilog << endl << ");" << endl << endl;
    }


    verilog << endl << "endmodule" << endl;
    verilog.close();
    log_->procEnd("Write Verilog",1);
}

void
ArtNetGen::writeSdc(const char* fileName) {
    log_->procBegin("Write Sdc",1);

    ofstream sdc(fileName);

    if(!sdc.good()) {
        cout << "Cannot open " << fileName << " file" << endl;
        return;
    }



    sdc << "###########################################" << endl;
    sdc << "# Created by NetGen (author: Daeyeon Kim) #" << endl;
    sdc << "###########################################" << endl;
    sdc << endl;

    sdc << "set sdc_version 2.1" << endl;
    sdc << endl;

  
    int resolution = 50;
    int unit = 1000;
    int synClkPeriod = (avgGateDelay_ * avgTopoOrder_) * unit;
    
    synClkPeriod -= synClkPeriod % resolution;
    //double clkRaising = 0;
    //double clkFalling = 0.5 * synClkPeriod;

    // unit --> ns
    double clkPeriod = 1.0 * synClkPeriod; 
    double ioDelay = 0.00 * synClkPeriod;

    //sdc << "set_units -time ns -resistance kOhm -capacitance pF -voltage V -current mA" << endl;
    sdc << "create_clock [get_ports clk] -period " << to_string(clkPeriod) << " -name clk" << endl;
    //"-waveform " << "{" << to_string(clkRaising*unit) << " " << to_string(clkFalling*unit) << "} -name clk" << endl;


    vector<Node*> primIns = artnet_->getPrimaryInputs();
    vector<Node*> primOuts = artnet_->getPrimaryOutputs();

    for(Node* inNode : primIns) {
        if(inNode->getType() == NodeType::ClockIn)
            //|| inNode->getType() == NodeType::ResetIn) 
            continue;
        
        sdc << "set_input_delay -max " << to_string(ioDelay) << " [get_ports " << inNode->getName() << "] -clock clk" << endl;
    }

    for(Node* outNode : primOuts) {
        sdc << "set_output_delay -max " << to_string(ioDelay) << " [get_ports " << outNode->getName() << "] -clock clk" << endl;
    }

    sdc.close();
    log_->procEnd("Write Sdc",1);
}




void
ArtNetGen::run() {
    if (specFile_ != "" ) {
        readSpec();
    }
    else {
        // spec file must be specified
        log_->errorQuit("Spec file must be specified", 1);
    }

    checkMasters();
    printAllVars();


    artnet_ = new Netlist(); 
    artnet_->setArtNetGen(this);
    artnet_->generate();


// update later... (crash here)
// somtime net has multiple primary outputs
//
/*
    vector<Net*> nets = artnet_->getNetlist();

    // dbChip create
    dbChip* chip = nullptr;
    if( db_->getChip() == nullptr) {
      chip = dbChip::create(db_);
    } else {
        chip = db_->getChip();
    }


    // dbBlock create
    dbBlock* block = nullptr;
    if( db_->getChip()->getBlock() == nullptr) {
      block = dbBlock::create(chip, topModule_.c_str()); 
    } else {
      block = db_->getChip()->getBlock();
    }

cout << "here1" << endl;

    db_->getTech()->setDbUnitsPerMicron(2000);
    block->setDefUnits(2000);
 
    unordered_map<Node*, dbInst*> node2inst;

    for(Net* net : nets) {
        dbNet* dbnet = dbNet::create(block, net->getName().c_str());

        vector<pair<Node*, string>> terms = net->getTerms();


cout << "here2" << endl;
cout << net->getName() << endl;

        for(int i=0; i < terms.size(); i++) {
cout << i << " here2-1" << endl;
            Node* node = terms[i].first;
            string termName = terms[i].second;
cout << node->getName() << " " << termName << endl;

            if(i==0) {
                switch(node->getType()) {
                    case NodeType::ClockIn:
                        dbnet->setSigType(dbSigType::CLOCK); break;
                    case NodeType::ResetIn:
                        dbnet->setSigType(dbSigType::RESET); break;
                    default:
                        dbnet->setSigType(dbSigType::SIGNAL); break;
                }
            }


cout << "here3" << endl;
            if(node->isPIO()) {
                dbBTerm* bTerm = dbBTerm::create(dbnet, dbnet->getConstName()); 

cout << "here4" << endl;
                switch(node->getType()) {
                    case NodeType::ClockIn:
                        bTerm->setSigType(dbSigType::CLOCK); 
                        bTerm->setIoType(dbIoType::INPUT); 
                        break;
                    case NodeType::ResetIn:
                        bTerm->setSigType(dbSigType::RESET); 
                        bTerm->setIoType(dbIoType::INPUT);
                        break;
                    case NodeType::PrimaryIn:
                        bTerm->setSigType(dbSigType::SIGNAL);
                        bTerm->setIoType(dbIoType::INPUT);
                        break;
                    case NodeType::PrimaryOut:
                        bTerm->setSigType(dbSigType::SIGNAL);
                        bTerm->setIoType(dbIoType::OUTPUT);
                        break;
                }                   
cout << "here4-1" << endl;

            } else {
cout << "here5" << endl;
                if(node2inst.find(node) == node2inst.end()) {
                    dbInst* inst = dbInst::create(block, node->getDbMaster(), node->getName().c_str());
                    node2inst[node] = inst;
                }

                dbInst* inst = node2inst[node];
                dbITerm* iTerm = inst->findITerm(termName.c_str());
                dbITerm::connect(iTerm, dbnet);
            }
        }
cout << "here6" << endl;
    }
    // get timing info
    // sta_->updateTiming(false);
    // sta::dbNetwork* network = sta_->getDbNetwork();
    // sta::Graph* graph = sta_->ensureGraph();
cout << "here7" << endl;
*/
}

// parse the spec file
void
ArtNetGen::readSpec() {
    log_->procBegin("Read Spec",1);

    ifstream ifs(specFile_);
    if( !ifs.good() ) {
        log_->errorQuit("Cannot open spec file " + specFile_, 3);
    }

    string line;

    // modified by dykim
    int maxFi = INT_MIN;
    int maxFo = INT_MIN;
    int maxBbox = INT_MIN;
    int maxWindow = INT_MIN;

    vector<DistributionInfo> fanIns;
    vector<DistributionInfo> fanOuts;
    vector<DistributionInfo> netBboxes;
    vector<DistributionInfo> windows;

    int totInstCnt = 0;

    while(getline(ifs, line)) {
        stringstream ss(line);
        string modeStr;
        ss >> modeStr;

        if( modeStr == "CELL_COUNT" ) {
            int value;
            ss >>  value;
            //instanceCnt_ = value;

        } else if ( modeStr == "COMB_RATIO" ) {
          
            double value;
            ss >>  value;

            combRatio_ = value;

        } else if( modeStr == "ONLY_USE" ) {
            string masterName;
            ss >> masterName;

            dbMaster* master = db_->findMaster( masterName.c_str() ); 
            if( !master ) {
                log_->errorQuit("Cannot find master cell " + masterName, 4);
            }

            double ratio;
            ss >> ratio;

            masters_.push_back(MasterInfo(master, ratio));
        }
        else if( modeStr == "FANIN" ) { 
            int value, target;
            double ratio;

            ss >> value >> target >> ratio;


            totInstCnt += target;


            maxFi = max(value, maxFi);
            fanIns.push_back(DistributionInfo(value, target, 0, ratio));
        }
        else if( modeStr == "FANOUT" ) {
            int value, target;
            double ratio;

            ss >> value >> target >> ratio;
            maxFo = max(value, maxFo);
            fanOuts.push_back(DistributionInfo(value, target, 0, ratio));
        }
        else if( modeStr == "WINDOW" ) { 
            int value, target;
            double ratio;

            ss >> value >> target >> ratio;
            maxWindow = max(value, maxWindow);
            windows.push_back(DistributionInfo(value, target, 0, ratio));
        }
        else if( modeStr == "NETBBOX" ) { 
            int value, target;
            double ratio;

            ss >> value >> target >> ratio;
            maxBbox = max(value, maxBbox);
            netBboxes.push_back(DistributionInfo(value, target, 0, ratio));
        }
        else if( modeStr == "IPAD_COUNT" ) {
            int value;
            ss >> value;
            setInputPinCnt(value);
        }
        else if( modeStr == "OPAD_COUNT" ) {
            int value;
            ss >> value;
            setOutputPinCnt(value);
        }
        //else if (modeStr == "SYN_CLK_PERIOD" ) {
        //    double value;
        //    ss >> value;
        //    setSynClkPeriod(value); 
        //}
        else if (modeStr == "AVG_GATE_DELAY" ) {
            double value;
            ss >> value;
            setAvgGateDelay(value);
        }
        else if (modeStr == "AVG_TOPO_ORDER" ) {
            double value;
            ss >> value;
            setAvgTopoOrder(value);
        }



    }


    // instance count from SPEC
    instanceCnt_ = totInstCnt;

    // BIN SQRT
    int binCnt = ceil(1.0 * instanceCnt_ / sqrt(instanceCnt_)); 



    fanIns_=fanIns;
    fanOuts_ = fanOuts;
    netBboxes_ = netBboxes;
    edges_ = windows;
    maxFanin_ = maxFi;
    maxFanout_ = maxFo;

  // initialize distribution
  fiDist_.init( 0, maxFi, getInstanceCnt(), fanIns );
  foDist_.init( 0, maxFo, getInstanceCnt(), fanOuts );
  bboxDist_.init( 0, maxBbox, getInstanceCnt(), netBboxes );
  int totalEdgeCnt = ceil(1.0 * getInstanceCnt() * fiDist_.targetAvg());
  // avg. net degree = (avg. # fanin)
  // total edge count = (avg. net degree) x (# instances)
  edgeDist_.init( 0, maxWindow, totalEdgeCnt, windows);

  fiDist_.setDescription("fanin distribution");
  foDist_.setDescription("fanout distribution");
  bboxDist_.setDescription("bbox distribution");
  edgeDist_.setDescription("edgelength distribution");


  ifs.close();
  log_->infoInt("NumMasters", masters_.size());
  //log_->infoInt("NumFanIns", fanIns_.size());
  //log_->infoInt("NumFanOuts", fanOuts_.size());
  //log_->infoInt("NumWindows", windows_.size());
  //log_->infoInt("NumNetBBoxes", netBBoxes_.size());

  log_->procEnd("Read Spec",1);
}

void
ArtNetGen::printAllVars() {
  if( verbose_ >= 5) {
    log_->infoString("Master");
    for(auto& master : masters_) {
      master.print(); 
    }

    //log_->infoString("FanIn");
    //fiDist_.print();

    //log_->infoString("FanOut");
    //foDist_.print();

    //log_->infoString("netBBox");
    //bboxDist_.print();

    //log_->infoString("Window");
    //edgeDist_.print();



    /*
    for(auto& fanIn : fanIns_ ) {    
      fanIn.print();
    }

    log_->infoString("FanOut");
    for(auto& fanOut : fanOuts_) {
      fanOut.print();
    }

    log_->infoString("Window");
    for(auto& window: windows_) {
      window.print();
    }
    
    log_->infoString("netBBox");
    for(auto& netBBox: netBBoxes_) {
    netBBox.print();
    }
    */
  }

  log_->infoInt("TargetInstCnt", instanceCnt_);
  log_->infoFloat("CombinationRatio", combRatio_);
  log_->infoInt("InputPinCnt", inputPinCnt_);
  log_->infoInt("OutputPinCnt", outputPinCnt_);
  //log_->infoInt("BinSqrt", binSqrt_);
  //log_->infoInt("MaxChainDepth", maxChainDepth_);
  //log_->infoFloat("SynClkPeriod", synClkPeriod_);
  log_->infoFloat("AvgTopoOrder", avgTopoOrder_);
  log_->infoFloat("AvgGateDelay", avgGateDelay_);

}

void
ArtNetGen::checkMasters() {
    
    unordered_map<int,vector<MasterInfo>> fi2TotCombMasters;
    unordered_map<int,vector<MasterInfo>> fi2TotSequMasters;

    unordered_map<int,vector<MasterInfo>> fi2OnlyUseCombMasters;
    unordered_map<int,vector<MasterInfo>> fi2OnlyUseSequMasters;

    dbSet<dbLib> libs = db_->getLibs();

    for(auto itl = libs.begin(); itl != libs.end(); itl++) {
        dbSet<dbMaster> masters = itl->getMasters();

        for(auto itm = masters.begin(); itm != masters.end(); itm++) {
            
            string masterName = itm->getName();

            dbMaster* master = db_->findMaster(masterName.c_str());
            dbSet<dbMTerm> terms = master->getMTerms();

            int inSigCnt = 0;
            int inClkCnt = 0;
            int inRstCnt = 0;
            int inScanCnt = 0;
            int outSigCnt = 0;
            int outClkCnt = 0;
            int outRstCnt = 0;
            int outScanCnt = 0;

            for(auto it = terms.begin(); it != terms.end(); it++) {

                string termName = it->getName();

                if(termName == "si" || termName == "ssb") {
                    inScanCnt++;
                } else {
                    dbSigType sigType = it->getSigType();
                    dbIoType ioType = it->getIoType();

                    if(sigType.getValue() == dbSigType::SIGNAL) {
                        if(ioType.getValue() == dbIoType::INPUT) {
                            inSigCnt++;
                        } else if(ioType.getValue() == dbIoType::OUTPUT) {
                            outSigCnt++;
                        }
                        //cout << termName << " " << sigType.getString() << " " << ioType.getString() << endl;
                    } else if(sigType.getValue() == dbSigType::CLOCK) {
                        if(ioType.getValue() == dbIoType::INPUT) {
                            inClkCnt++;
                        } else if(ioType.getValue() == dbIoType::OUTPUT) {
                            outClkCnt++;
                        }
                    } else if(sigType.getValue() == dbSigType::RESET) {
                        if(ioType.getValue() == dbIoType::INPUT) {
                            inRstCnt++;
                        } else if(ioType.getValue() == dbIoType::OUTPUT) {
                            outRstCnt++;
                        }
                    } else if(sigType.getValue() == dbSigType::SCAN) {
                        if(ioType.getValue() == dbIoType::INPUT) {
                            inScanCnt++;
                        } else if(ioType.getValue() == dbIoType::OUTPUT) {
                            outScanCnt++;
                        }
                    } else {

                    }
                }
            }

        cout << master->getName() << " inSig " 
             << inSigCnt << " outSig " << outSigCnt << " inClk " << inClkCnt << " outClk " << outClkCnt << " inRst " 
             << inRstCnt << " outRst " << outRstCnt << " inScn " << inScanCnt << " outScn " << outScanCnt << endl;
            if(outSigCnt == 0)
                continue;

            if(inScanCnt > 0) {
                cout << "current version does not support scan-chain cell... (" << master->getName() << ")" <<  endl;
                continue;
            }

            if(inClkCnt > 0) {
                fi2TotSequMasters[inSigCnt].push_back(MasterInfo(master, 1.0));
            } else {
                // combinational cell
                fi2TotCombMasters[inSigCnt].push_back(MasterInfo(master, 1.0));
            }
        }
    }

    for(MasterInfo& info : masters_) {
        dbMaster* master = info.master();
        float ratio = info.ratio();
        dbSet<dbMTerm> terms = master->getMTerms();

        int inSigCnt = 0;
        int inClkCnt = 0;
        int inRstCnt = 0;
        int inScanCnt = 0;
        int outSigCnt = 0;
        int outClkCnt = 0;
        int outRstCnt = 0;
        int outScanCnt = 0;

        for(auto it = terms.begin(); it != terms.end(); it++) {

            string termName = it->getName();

            dbSigType sigType = it->getSigType();
            dbIoType ioType = it->getIoType();

            if(termName == "si" || termName == "ssb") {
                inScanCnt++;
            } else {
                if(sigType.getValue() == dbSigType::SIGNAL) {
                    if(ioType.getValue() == dbIoType::INPUT) {
                        inSigCnt++;
                    } else if(ioType.getValue() == dbIoType::OUTPUT) {
                        outSigCnt++;
                    }
                    //cout << termName << " " << sigType.getString() << " " << ioType.getString() << endl;
                } else if(sigType.getValue() == dbSigType::CLOCK) {
                    if(ioType.getValue() == dbIoType::INPUT) {
                        inClkCnt++;
                    } else if(ioType.getValue() == dbIoType::OUTPUT) {
                        outClkCnt++;
                    }
                } else if(sigType.getValue() == dbSigType::RESET) {
                    if(ioType.getValue() == dbIoType::INPUT) {
                        inRstCnt++;
                    } else if(ioType.getValue() == dbIoType::OUTPUT) {
                        outRstCnt++;
                    }
                } else if(sigType.getValue() == dbSigType::SCAN) {
                    if(ioType.getValue() == dbIoType::INPUT) {
                        inScanCnt++;
                    } else if(ioType.getValue() == dbIoType::OUTPUT) {
                        outScanCnt++;
                    }
                } else {

                }
            }
        }

        if(outSigCnt == 0)
            continue;

        if(inScanCnt > 0) {
            cout << "current version does not support scan-chain cell... (" << master->getName() << ")" <<  endl;
            continue;
        }

        if(inClkCnt > 0) {
            // sequential cell
            fi2OnlyUseSequMasters[inSigCnt].push_back(MasterInfo(master,ratio));
        } else {
            // combinational cell
            fi2OnlyUseCombMasters[inSigCnt].push_back(MasterInfo(master,ratio));
        }        
    }

    vector<MasterInfo> masterInfos;

    for(int x=1; x <= fiDist_.xMax(); x++) {
        if(fi2OnlyUseCombMasters.find(x) != fi2OnlyUseCombMasters.end()) {
            masterInfos.insert( masterInfos.end(), fi2OnlyUseCombMasters[x].begin(), fi2OnlyUseCombMasters[x].end());
        } else {
            masterInfos.insert( masterInfos.end(), fi2TotCombMasters[x].begin(), fi2TotCombMasters[x].end());
        }

        if(fi2OnlyUseSequMasters.find(x) != fi2OnlyUseSequMasters.end()) {
            masterInfos.insert(masterInfos.end(), fi2OnlyUseSequMasters[x].begin(), fi2OnlyUseSequMasters[x].end());
        } else { 
            masterInfos.insert( masterInfos.end(), fi2TotSequMasters[x].begin(), fi2TotSequMasters[x].end() );
        }

    
        //cout << "Available masters where # of fanins = " << x << endl; 
        //for(MasterInfo& info : masterInfos) {
        //    cout << "   - " << info.master()->getName() << endl;
        //}

    
    }

    masters_ = masterInfos;


    


}

void
ArtNetGen::writeVerilog() {
}

void
ArtNetGen::setDb(odb::dbDatabase* db) {
  db_ = db;
}

void
ArtNetGen::setSta(sta::dbSta* sta) {
  sta_ = sta;
}

//void
//ArtNetGen::setInstanceCnt(int instanceCnt) {
//  instanceCnt_ = instanceCnt;
//}
//
//void
//ArtNetGen::setInputPinCnt(int inputPinCnt) {
//  inputPinCnt_ = inputPinCnt;
//}
//
//void
//ArtNetGen::setOutputPinCnt(int outputPinCnt) {
//  outputPinCnt_ = outputPinCnt; 
//}
//
//void
//ArtNetGen::setSynClkPeriod(double clkPeriod) {
//    synClkPeriod_ = clkPeriod;
//}
//
void
ArtNetGen::setAvgGateDelay(double avgDelay) {
    cout << "avg. gate delay changes from " << avgGateDelay_ << " to " << avgDelay << endl;

    avgGateDelay_ = avgDelay;
}
//
//void
//ArtNetGen::setCombRatio(double combRatio) {
//  combRatio_ = combRatio_;
//}
//
//void
//ArtNetGen::setSpecFile(const char* specFile) {
//  specFile_ = specFile;
//}
//
//void
//ArtNetGen::setOutFile(const char* outFile) {
//  outFile_ = outFile;
//}
//
//void
//ArtNetGen::setVerbose(int verbose) {
//  verbose_ = verbose;
//}
//
//void
//ArtNetGen::setTopModule(const char* topModule) {
//    topModule_ = topModule;
//}
//
//
//// getter
//int ArtNetGen::getInstanceCnt() const {
//    return instanceCnt_;
//}
//
//int ArtNetGen::getInputPinCnt() const {
//    return inputPinCnt_;
//}
//
//int ArtNetGen::getOutputPinCnt() const {
//    return outputPinCnt_;
//}
//
//double ArtNetGen::getSynClkPeriod() const {
//    return synClkPeriod_;
//}
//
//
//double ArtNetGen::getAvgGateDelay() const {
//    return avgGateDelay_;
//}
//double ArtNetGen::getCombRatio() const {
//    return combRatio_; 
//}



}
