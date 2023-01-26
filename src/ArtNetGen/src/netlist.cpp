#include "artnetgen/artNetGen.h"
#include "netlist.h"
#include "bin.h"
#include "node.h"

#include <algorithm>
#include <math.h>
#include <limits.h>
#include <cfloat>
#include <queue>
#include "opendb/db.h"
#include <chrono>

//#include "opendb/dbSet.h"


namespace artnetgen {
//using std::cout;
//using std::endl;
//using std::vector;
//using std::max;
//using std::min;
//using std::string;
//using std::find;
//using std::unordered_map;
//using std::queue;
//using std::make_pair;
//using std::ceil;
//using std::to_string;
using namespace std;
using namespace odb;



void
Netlist::generate() {


    chrono::system_clock::time_point start;
    chrono::duration<double> runtime;
    
    // 1. initialize
    start = chrono::system_clock::now();
    initialize();
    runtime = chrono::system_clock::now() - start;
    cout << "[Info] Graph initialization finished (" << runtime.count() << " sec)" << endl;
    // 2. set primary I/O nodes
    start = chrono::system_clock::now();
    setPrimaryIO();
    // 3. create edge considering target distributions
    distMatching();
    runtime = chrono::system_clock::now() - start;
    cout << "[Info] Graph construction finished (" << runtime.count() << " sec)" << endl;
    //checkUnconnected();
    // 4. insert sequential logic cells
    start = chrono::system_clock::now();
    initOnlyUseMasters();
    timingPathConstruction_v1();
    runtime = chrono::system_clock::now() - start;
    cout << "[Info] Timing path construction finished (" << runtime.count() << " sec)" << endl;
    //checkUnconnected();
    // 5. map standard cell
    start = chrono::system_clock::now();
    technologyMapping();
    runtime = chrono::system_clock::now() - start;
    cout << "[Info] Technology mapping finished (" << runtime.count() << " sec)" << endl;

    summaryDesign();
}


void 
Netlist::summaryDesign() {
    vector<Node*> inUnconnected;
    vector<Node*> outUnconnected;

    unordered_map<Node*, int> topoOrder = topologicalSort();

    int maxTopoOrder = 0;

    int numCCs = 0;
    int numFFs = 0;
    int numPIs = 0;
    int numPOs = 0;


    for(int i=0; i < nodes_.size(); i++) {
        Node* node = nodes_[i];


        if( node->getType() == NodeType::PrimaryIn) {
            if( node->numFanins() != 0 ) {
                cout << "#fanin of primary input is not zero! (" << node->numFanins() << ")" <<  endl;
                exit(0);
            }
        } else if( node->getType() == NodeType::PrimaryOut) {
            if( node->numFanins() != 1 ) {
                cout << "#fanin of primary output is not one! (" << node->numFanins() << ")" <<  endl;
                exit(0);
            }

            if( node->numFanouts() != 0 ) {
                cout << "#fanout of primary output is not zero! (" << node->numFanouts() << ")" << endl;
                exit(0);
            }
        } else {
            if( node->numFanins() == 0 )
                inUnconnected.push_back(node);

            if( node->numFanouts() == 0 )
                outUnconnected.push_back(node);
        }

        int x = node->x();
        int y = node->y();
        int order = topoOrder[node];

        //cout << i << "-th node at (" << x << " " << y << ") -> order " << order << endl;

        switch(node->getType()) {
            case NodeType::Combinational: 
                numCCs++; break;
            case NodeType::Sequential:
                numFFs++; break;
            case NodeType::PrimaryIn:
                numPIs++; break;
            case NodeType::PrimaryOut:
                numPOs++; break;
            default: break;
        }

        maxTopoOrder = max(maxTopoOrder, order);

        
        /*
        cout << node->getName();
        if(node->getType() == NodeType::Combinational || node->getType() == NodeType::Sequential)
            cout << " (" << node->getDbMaster()->getName() << ")" << endl;
        else 
            cout << endl;
        for(Node* source : node->getSources()) {
            cout << "   " << node->getTerm(source) << " -> " << source->getName() << endl;
        }
        cout << endl;
        */
        

    
    
    }
    
    float seqRatio = 1.0 * numFFs / (numCCs + numFFs);


    cout << "# input unconnected nodes : " << inUnconnected.size() << endl;
    cout << "# output unconnected nodes : " << outUnconnected.size() << endl;
    cout << "Maximum topological order : " << getMaxTopologicalOrder() << endl;
    cout << "Average topological order : " << getAvgTopologicalOrder() << endl;
    cout << "# of combinational nodes : " << numCCs << endl;
    cout << "# of sequential nodes : " << numFFs << endl;
    cout << "# of primary input nodes : " << numPIs << endl;
    cout << "# of primary output nodes : " << numPOs << endl;
    cout << "Sequential ratio : " << seqRatio << " (" << 1- ang_->getCombRatio() << ")" << endl;
    //cout << "primary input/output will be connected to the unconnected nodes, first" << endl;







}



void
Netlist::setPrimaryIO() {

    int inputPinCnt = ang_->getInputPinCnt();
    int outputPinCnt = ang_->getOutputPinCnt();
    int x,y;
    bool done;

    x = 0, y = 0;
    done = false;


    while(true) {
        Bin* headBin = getBin(x,y);
        vector<Node*> candidates = headBin->fi2Nodes(0);

        for(int i=0; i < candidates.size(); i++) {
            
            Node* node = candidates[i];
            node->setType(NodeType::PrimaryIn);

            primIns_.push_back(node);

            if(primIns_.size() >= inputPinCnt) {
                done = true;
                break;
            }
        }

        if(!done) {
            x++;
            if( x >= layoutDimX_) {
                cout << "# of primary inputs is too large!" << endl;
                exit(0);
            }

        } else {
            break;
        }
    }

    x = layoutDimX_-1, y=layoutDimY_-1;
    done = false;

    while(true) {
        Bin* tailBin = getBin(x,y);
        vector<Node*> candidates = tailBin->fo2Nodes(0);

        for(int i=0; i < candidates.size(); i++) {
            Node* node = candidates[i];
            node->setType(NodeType::PrimaryOut);

            primOuts_.push_back(node);

            if(primOuts_.size() >= outputPinCnt) {
                done = true;
                break;
            }
        }

        if(!done) {
            y--;
            if( y < 0 ) {
                cout << "# of primary outputs is too large!" << endl;
                exit(0);
            }
        } else {
            break;
        }
    }

    cout << "# of primary inputs is " << inputPinCnt << endl;

    for(int i=0; i < primIns_.size(); i++) {
        Node* primIn = primIns_[i]; 
        
        string ioName = "in" + to_string(i);
        primIn->setName(ioName);

        cout << " - " << i << "-th primary input is in (" << primIn->x() << " " << primIn->y() << ")" << endl;
    }

    cout << "# of primary outputs is " << outputPinCnt << endl;

    for(int i=0; i < primOuts_.size(); i++) {
        Node* primOut = primOuts_[i];
        string ioName = "out" + to_string(i);
        primOut->setName(ioName);
        cout << " - " << i << "-th primary output is in (" << primOut->x() << " " << primOut->y() << ")" << endl;
    }




    /*
    int inputPinCnt = ang_->getInputPinCnt();
    int outputPinCnt = ang_->getOutputPinCnt();

    cout << "# Input primary inputs : " <<  inputPinCnt << endl;
    cout << "# Output primary outputs : " << outputPinCnt << endl;


    if( inputPinCnt < inUnconnected.size() ) {
        cout << "# pimary inputs is smaller than # input unconnected nodes" << endl;
        cout << "revise inputPinCnt (" << inputPinCnt << " -> " << inUnconnected.size() << ")" << endl;
        inputPinCnt = inUnconnected.size();
    }

    if( outputPinCnt < outUnconnected.size() ) {
        cout << "# pimary outputs is smaller than # output unconnected nodes" << endl;
        cout << "revise outputPinCnt (" << outputPinCnt << " -> " << outUnconnected.size() << ")" << endl;
        outputPinCnt = outUnconnected.size();
    }


    


    vector<Node*> roots = inUnconnected;
    vector<Node*> leafs = outUnconnected;
    vector<Node> inNodes(inputPinCnt);
    vector<Node> outNodes(outputPinCnt);


    // create primary input
    for(int i=0; i < inputPinCnt; i++) {

        Node* inN = &inNodes[i];
        if(i!=inputPinCnt-1) {
            inN->setType(NodeType::PrimaryIn);
        
            // randomly choose 
            int x = 0;
            int y = rand() % layoutDimY_;

            int 
        
        } else {
            inN->setType(NodeType::ClockIn);

        }
    }

    // create primary output
    for(int i=0; i < outputPinCnt; i++) {

    }
    */



}


void
Netlist::initialize() {
    //ang_ = ang;
    //layoutDimX_ = ang_->binSqrt(); // BIN 2D Grid 너비 (x-축 bin count)
    //layoutDimY_ = ang_->binSqrt(); // BIN 2D Grid 높이 (y-축 bin count)
                                      // binSqrt --> sqrt(#bins) 라고 생각하면됨


    double combRatio = ang_->getCombRatio();              // Nonclocking Cell 비율
    int instanceCnt = ang_->getInstanceCnt();                          // Total instance 개수 ( = CC + FF )
    double discountFactor = 0.80;
    int combCellCnt = std::ceil( instanceCnt * combRatio);  
    int flipflopCnt = instanceCnt * (1-combRatio);
    int totalBinCnt = ceil(sqrt(instanceCnt));

    // Init layout dimension
    layoutDimX_ = 1;
    layoutDimY_ = 1;

    while(layoutDimX_ * layoutDimY_ < totalBinCnt) {
        if(layoutDimX_ > layoutDimY_)
            layoutDimY_++;
        else
            layoutDimX_++;
    }


    int initCellCnt = 0.80 * instanceCnt - 0.2 * (1-combRatio) * instanceCnt;

    cout << "Layout dimension (" << layoutDimX_ << " " << layoutDimY_ << ")" << endl;
   
    int binCnt = layoutDimX_ * layoutDimY_;
    //int avgNodeCnt = ceil( 1.0* instanceCnt / binCnt );             // BIN당 평균 instance (node) 개수

    
    
    //int avgNodeCnt = ceil( 1.0 * combCellCnt / binCnt );             // BIN당 평균 instance (node) 개수
    int avgNodeCnt = ceil( 1.0 * initCellCnt / binCnt );             // BIN당 평균 instance (node) 개수
   



    int maxFanin = ang_->getMaxFanin();
    int maxFanout = ang_->getMaxFanout();
    int maxEdgeLength = layoutDimX_ + layoutDimY_;
    // copy (확장성을위해 netlist의 spec은 독립적으로 할당)
    fiDist_.setDescription("fanin distribution");
    foDist_.setDescription("fanout distribution");
    bboxDist_.setDescription("net bbox distribution");
    fiDist_.init(0, maxFanin, initCellCnt, ang_->getFaninDistInfo());
    foDist_.init(0, maxFanout, initCellCnt, ang_->getFanoutDistInfo());
    bboxDist_.init(0, maxEdgeLength, initCellCnt, ang_->getBboxDistInfo());
    int totEdgeCnt = ceil(1.0*initCellCnt*fiDist_.targetAvg());
    edgeDist_.init(0, maxEdgeLength, totEdgeCnt, ang_->getEdgeDistInfo());
    //fiDist_.init(0, maxFanin, combCellCnt, ang_->getFaninDistInfo());
    //foDist_.init(0, maxFanout, combCellCnt, ang_->getFanoutDistInfo());
    //bboxDist_.init(0, maxEdgeLength, combCellCnt, ang_->getBboxDistInfo());
    //int totEdgeCnt = ceil(1.0*combCellCnt*fiDist_.targetAvg());
    //edgeDist_.init(0, maxEdgeLength, totEdgeCnt, ang_->getEdgeDistInfo());


    //foDist_ = ang_->foDist();
    //bboxDist_ = ang_->bboxDist();
    //edgeDist_ = ang_->edgeDist();

    // Bin 생성 (Stor)
    binStor_ = vector<Bin>(binCnt);

    // Bin 생성 (Pointer)
    for(int i=0; i < binCnt; i++) {
        int x = i % layoutDimX_;
        int y = i / layoutDimX_;
      
        //cout << x << " " << y << " -> " << i << endl;
        bins_.push_back(&binStor_[i]);
        bins_.back()->setCoord(x,y); 
        bins_.back()->init(fiDist_.xMax(), foDist_.xMax());
    }

    // Node 생성(Stor)
    //nodeStor_.reserve( 3* instanceCnt );
    //nodeStor_ = vector<Node>( instanceCnt );

    // Node 생성 (Pointer)
    for(int i=0; i < initCellCnt; i++) {
        Node* n = new Node();
        nodes_.push_back(n);
        //Node *n;
        //nodeStor_.push_back(n);
        //nodes_.push_back(&nodeStor_[i]);
    }

    // x-y coordinate 할당 (Node -> Bin)
    for(int i=0; i < binCnt; i++) {
        int x = i % layoutDimX_;
        int y = i / layoutDimX_;
        //cout << x << " " << y << endl;
        Bin* bin = getBin(x, y);

        for(int j=avgNodeCnt*i; j < avgNodeCnt*(i+1) && j < initCellCnt; j++) {
            string cellName = "c" + to_string(j);
            Node* node = nodes_[j];
            node->setType(NodeType::Combinational);
            node->setName(cellName);
            bin->addNode(node);
        }
    }

    cout << "finished initialize" << endl;
    print();
}

void Netlist::setArtNetGen(ArtNetGen* ang) {
    ang_ = ang;
}

Netlist::Netlist() {
}
Netlist::~Netlist() {

}

Bin* Netlist::getBin(int x, int y) {
    int idx = x + y * layoutDimX_;
   
    Bin* bin = bins_[idx];
    if (bin->x() != x || bin->y() != y) {
        cout << "idx : " << idx << "(" << bin->x() << " " << bin->y() <<") (" << x << " " << y << ")" << endl;
        exit(0);
    }

    return bins_[idx];
}


void
Netlist::print() {
    foDist_.print();

    fiDist_.print();

    bboxDist_.print();

    //edgeDist_.print();
    //
    //

   
    int multiPortConn = 0;
    for(Node* node : nodes_) {
        if(node->hasMultiPortConn())
            multiPortConn++;
    }

    cout << "# of multiple port connection cells = " << multiPortConn << endl;


}

void
Netlist::connect(Node* srcNode, Node* sinkNode) {
    //cout << "connect" << endl;
    int edgeLen = abs(srcNode->x() - sinkNode->x()) + abs(srcNode->y() - sinkNode->y());
    // decrement previous fanin, fanout 
    fiDist_.decr(sinkNode->numFanins());
    foDist_.decr(srcNode->numFanouts());
    bboxDist_.decr(srcNode->bboxSize());
    // add source and sink for each node
    srcNode->addSink(sinkNode);
    sinkNode->addSource(srcNode);
    // increment current fanin, fanout
    fiDist_.incr(sinkNode->numFanins());
    foDist_.incr(srcNode->numFanouts());
    bboxDist_.incr(srcNode->bboxSize());
    edgeDist_.incr(edgeLen);
    
    Bin* srcBin = srcNode->getBin();
    Bin* sinkBin = sinkNode->getBin();
    // update information of srcNode in the bin
    srcBin->update(srcNode);
    sinkBin->update(sinkNode);
}


vector<Net*> Netlist::getNetlist() {
    return nets_;
}

vector<Node*> Netlist::getPrimaryInputs() {
    return primIns_;
}

vector<Node*> Netlist::getPrimaryOutputs() {
    return primOuts_;
}

vector<Node*> Netlist::getNodes() {
    return nodes_;
}


};
