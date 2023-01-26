#include "opendb/db.h"
#include "node.h"
#include "bin.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <unordered_map>

namespace artnetgen {

using std::ceil;
using std::min;
using std::max;
using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::make_pair;
using std::find;
using std::pair;
using std::unordered_map;

using namespace odb;

Net::Net() {

}

//Net::Net(string name) {
//    name_ = name;
//}

//Net::Net(const Net& net) :
//    name_(net.name_), terms_(net.terms_) {}


Net::~Net() {

}
string Net::getName() {
    return name_;
}

vector<pair<Node*, string>> Net::getTerms() {
    return terms_;
}


void Net::setName(string name) {
    name_ = name;
}

void Net::addTerm(Node* node, string termName) {
    if(termName == "PIN") {
       terms_.push_back( make_pair(node, node->getName()) ); 
    } else {
       terms_.push_back( make_pair(node, termName) );
    }
}

void Net::print() {
    cout << "- " << name_ << endl;

    int endlCnt = 0;
    for(int i=0; i < terms_.size(); i++) {
        Node* node = terms_[i].first;
        string termName = terms_[i].second;

        if(node->getType() == NodeType::Combinational || node->getType() == NodeType::Sequential) {
            cout << "  ( " << node->getName() << " " << termName << " )"; // << endl;
        } else {
            cout << "  ( PIN " << termName << " )"; // << endl;
        }

        endlCnt++;

        if(endlCnt == 3) {
            endlCnt = 0;
            cout << endl;
        }
    }

    if( endlCnt != 0 )
        cout << endl;
    cout << endl;
}


Bin* Node::getBin() {
    return bin_;
}


int Node::numFanins() const { 
    return sources_.size(); 
}

int Node::numFanouts() const { 
    return sinks_.size(); 
}

int Node::x() const { 
    return bin_->x(); 
}

int Node::y() const { 
    return bin_->y(); 
}

int Node::lx() const { 
    return lx_; 
}

int Node::ly() const { 
    return ly_; 
}

int Node::ux() const { 
    return ux_; 
}

int Node::uy() const { 
    return uy_; 
}

int Node::bboxSize() const { 
    return ux_ - lx_ + uy_ - ly_; 
}

void Node::setName(string name) {
    name_ = name;
}

string Node::getName() {
    return name_;
}

void Node::setType(NodeType type) {
    type_ = type;
}

void Node::setBin(Bin* bin) {
    bin_ = bin;
    // init bbox
    setBbox(bin_->x(), bin_->y(), bin_->x(), bin_->y());
}

void Node::setBbox(int lx, int ly, int ux, int uy) {
    lx_ = lx, ly_ = ly, ux_ = ux, uy_ = uy;    
}

void Node::addSource(Node* srcNode) {
#ifdef DEBUG
    vector<Node*>::iterator it = find(sources_.begin(), sources_.end(), srcNode);
    if(it == sources_.end()) {
        cout << "Source node already exists!" << endl;
        exit(0);
    }
#endif        

    sources_.push_back(srcNode);
}

void Node::addSink(Node* sinkNode) {
    sinks_.push_back(sinkNode);
    int sx = sinkNode->x();
    int sy = sinkNode->y();

    lx_ = min(sx, lx_);
    ly_ = min(sy, ly_);
    ux_ = max(sx, ux_);
    uy_ = max(sy, uy_);
}

void Node::addSources(vector<Node*> sources) {

    for(auto source : sources) {
        sources_.push_back(source);
        source->addSink(this);
    }

    
    //sources_.insert(sources_.end(), sources.begin(), sources.end());
}

void Node::addSinks(vector<Node*> sinks) {
    for(auto sink : sinks) {
        sinks_.push_back(sink);
        sink->addSource(this);
    }
    //sinks_.insert(sinks_.end(), sinks.begin(), sinks.end());
}


void Node::removeAllSinks() {
    for(auto sink : sinks_)
        sink->removeSource(this);
    sinks_.clear(); // = vector<Node*>();
}

void Node::removeAllSources() {
    for(auto source : sources_)
        source->removeSink(this);
    sources_.clear(); // = vector<Node*>();
}

vector<Node*> Node::getSources() {
    return sources_;
}

vector<Node*> Node::getSinks() {
    return sinks_;
}

NodeType Node::getType() {
    return type_;
}

Node::Node() {

}
Node::~Node() {
    //std::cout << "node is free" << std::endl;
}
void Node::setDbMaster(dbMaster* master) {
    master_ = master;
}

dbMaster* Node::getDbMaster() {
    return master_;
}

void Node::mappingTerms(Node* clkIn, Node* rstIn) {
    
    //cout << "mapping term -1" << endl;
    //cout << name_ << endl;
    //cout << master_->getName() << endl;
    vector<Node*>::iterator sourceIter = sources_.begin();
    
    if(type_ == NodeType::PrimaryIn) {
        outSigTerms_.push_back("PIN"); //name_);
    } else if(type_ == NodeType::PrimaryOut) {
        inSigTerms_.push_back("PIN"); //name_);
    } else if(type_ == NodeType::ClockIn) {
        outSigTerms_.push_back("PIN");//name_);
    } else if(type_ == NodeType::ResetIn) {
        outSigTerms_.push_back("PIN"); //name_);
    } else {
        dbSet<dbMTerm> terms = master_->getMTerms();
        for(auto it = terms.begin(); it != terms.end(); it++) {
            dbSigType sigType = it->getSigType();
            dbIoType ioType = it->getIoType();
            //cout << "[" << name_ << "] " << master_->getName() << " " << it->getName() << " " << sigType.getString() << " " << ioType.getString() << endl;
            if(sigType.getValue() == dbSigType::SIGNAL) {
                if(ioType.getValue() == dbIoType::INPUT) {
                    inSigTerms_.push_back(it->getName());
                } else if(ioType.getValue() == dbIoType::OUTPUT) {
                    outSigTerms_.push_back(it->getName());
                }
            } else if(sigType.getValue() == dbSigType::CLOCK) {
                if(ioType.getValue() == dbIoType::INPUT) {
                    inClkTerms_.push_back(it->getName());
                }
            } else if(sigType.getValue() == dbSigType::RESET) {
                if(ioType.getValue() == dbIoType::INPUT) {
                    inRstTerms_.push_back(it->getName());
                }
            } else {

            }
        
        }
    }
    //cout << "mapping term -2" << endl;
    // for debug
    if( inSigTerms_.size() != sources_.size() ) {
        cout << "# of sources and # of mterms are not matched!" << endl;
        exit(0);
    }

    if( sinks_.size() > 0 && outSigTerms_.size() == 0 ) {
        cout << "# of output signal terms is zero!" << endl;
        exit(0);
    }


    //cout << "mapping term -3" << endl;


    for(int i=0; i < sources_.size(); i++) {
        //if(name_ == "c87") {
        //    cout << sources_[i] << " -> " << inSigTerms_[i] << endl;
        //}

        
        node2term_[sources_[i]] = inSigTerms_[i];
    }
    //cout << "mapping term -4" << endl;
    
    for(int i=0; i < outSigTerms_.size(); i++) {
        int numFanouts = ceil(1.0 * sinks_.size() / outSigTerms_.size());
        int beginIdx = i * numFanouts;
        int endIdx = min( (int) sinks_.size(), (i+1) * numFanouts );
        for(int j=beginIdx; j < endIdx; j++) {
            node2term_[sinks_[j]] = outSigTerms_[i]; 
        }
    }

    //cout << "mapping term -5" << endl;
    for(int i=0; i < inClkTerms_.size(); i++) {
        node2term_[clkIn] = inClkTerms_[i];
        clkIn->addSink(this);
        addSource(clkIn);
    }

    //cout << "mapping term -6" << endl;
    for(int i=0; i < inRstTerms_.size(); i++) {
        node2term_[rstIn] = inRstTerms_[i];
        rstIn->addSink(this);
        addSource(rstIn);
    }

    //cout << "mapping term -7" << endl;
    /*

    */


}

bool Node::hasConnection(Node* node) {
    vector<Node*>::iterator it = find(sinks_.begin(), sinks_.end(), node);
    if( it == sinks_.end() )
        return false;
    else
        return true;
}


string Node::getTerm(Node* node) {
    return node2term_[node];
}

void Node::removeSink(Node* target) {
    vector<Node*>::iterator it = find(sinks_.begin(), sinks_.end(), target);
    if(it != sinks_.end())
        sinks_.erase(it);
}

void Node::removeSource(Node* target) {
    vector<Node*>::iterator it = find(sources_.begin(), sources_.end(), target);
    if(it != sources_.end())
        sources_.erase(it);
}

bool Node::isPIO() const {
    switch(type_) {
        case NodeType::PrimaryIn: return true;
        case NodeType::PrimaryOut: return true;
        case NodeType::ClockIn: return true;
        case NodeType::ResetIn: return true;
        default: return false;
    }
}

bool Node::hasMultiPortConn() {
    
    unordered_map<Node*, int> weight;

    for(Node* node : sources_) {
        if(weight.find(node) == weight.end())
            weight[node] = 0;
        weight[node]++;
        if(weight[node] > 1)
            return true;
    }


    return false;
}



};
