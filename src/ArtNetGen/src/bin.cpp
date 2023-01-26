#include "bin.h"
#include "node.h"
#include <random>
#include <algorithm>
namespace artnetgen {

using std::vector;
using std::max;
using std::min;
using std::string;
using std::find;
using std::rand;
using std::cout;
using std::endl;
using std::make_pair;

Bin::Bin() {

}

Bin::~Bin() {

}

int Bin::x() const {
    return x_;
}

int Bin::y() const {
    return y_;
}

int Bin::numNodes() const {
    return nodes_.size();
}

vector<Node*> Bin::fi2Nodes(int fi) {
    return fi2Nodes_[fi];
}

vector<Node*> Bin::fo2Nodes(int fo) {
    return fo2Nodes_[fo];
}

Node* Bin::getRandomNode() {
    return nodes_[rand() % numNodes()];
}

void
Bin::setCoord(int x, int y) {
    x_ = x, y_ = y;
}

void
Bin::init(int maxFi, int maxFo) {
    fi2Nodes_ = vector<vector<Node*>>(maxFi+1);
    fo2Nodes_ = vector<vector<Node*>>(maxFo+1);
}


void
Bin::addNode(Node* node) {
    int fi = node->numFanins();
    int fo = node->numFanouts();
    nodes_.push_back(node);
    fi2Nodes_[fi].push_back(node);
    fo2Nodes_[fo].push_back(node);

    fi_[node] = fi;
    fo_[node] = fo;

    node->setBin(this);
}


void
Bin::update(Node* node) {
    //cout << "update" << endl;    
    vector<Node*> *tarVec;
    vector<Node*>::iterator it;

    // map에 저장된 node의 정보를 최신정보로 업데이트.
    if( node->numFanins() != fi_[node] ) {
//cout << "updated map (" << fi_[node] << " -> " << node->numFanins() << ")" << endl;
        tarVec = &fi2Nodes_[fi_[node]];
//int f1 = fi_[node];
//int n1 = fi2Nodes_[f1].size();
//int n3 = tarVec->size();

        it = std::find(tarVec->begin(), tarVec->end(), node);
        tarVec->erase(it);
        
//cout << "tarVec : " << n3 << "(" << tarVec->size() << ")" <<  endl;
        
        //it = std::find(fi2Nodes_[fi_[node]].begin(), fi2Nodes_[fi_[node]].end(), node);
        //fi2Nodes_[fi_[node]].erase(it); 
        fi_[node] = node->numFanins();
        fi2Nodes_[node->numFanins()].push_back(node);
//int n2 = fi2Nodes_[f1].size();
   

        //if(n1!=n2) {
        //    cout << "######## " << n1 << " " << n2 << endl;
        //    cout << "fi_[node] : " << fi_[node] << " (" << node->numFanins() << ")" << endl;
        //    exit(0);
        //}

    }

    if( node->numFanouts() != fo_[node] ) {

        //cout << "fo : " << node->numFanouts() << endl;
        tarVec = &fo2Nodes_[fo_[node]];
        it = std::find(tarVec->begin(), tarVec->end(), node);
        tarVec->erase(it);
        fo_[node] = node->numFanouts();
        fo2Nodes_[node->numFanouts()].push_back(node);
    }
}

};
