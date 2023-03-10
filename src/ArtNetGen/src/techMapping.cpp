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
//#include "opendb/dbSet.h"


namespace artnetgen {
using std::cout;
using std::endl;
using std::vector;
using std::max;
using std::min;
using std::string;
using std::find;
using std::unordered_map;
using std::queue;
using std::make_pair;
using std::ceil;
using std::to_string;

using namespace odb;

void
Netlist::initOnlyUseMasters() {
    int fiMax = fiDist_.xMax();
    fi2CombMasters_ = vector<vector<dbMaster*>>(fiMax+1);
    fi2SequMasters_ = vector<vector<dbMaster*>>(fiMax+1);

    vector<MasterInfo> masters = ang_->getMasterInfo();

    for(MasterInfo& info : masters) {
        dbMaster* master = info.master();
        float ratio = info.ratio();

        master2ratio_[master] = ratio;
        unordered_map<string, int> ioCount = getPortInfo(master);

        if(ioCount["output_signal"] == 0)
            continue;

        if(ioCount["input_scan"] > 0) {
            cout << "current version does not support scan-chain cell... (" << master->getName() << ")" <<  endl;
            continue;
        }

        if(ioCount["input_clock"] > 0) {
            // sequential cell
            fi2SequMasters_[ioCount["input_signal"]].push_back(master);       
        } else {
            // combinational cell
            fi2CombMasters_[ioCount["input_signal"]].push_back(master);
        }
    }


    //
    for(int i=0; i <= fiMax; i++) {

        cout << "# of fanins : " << i << endl;
        vector<dbMaster*> masters = fi2SequMasters_[i];
        for(dbMaster* master : masters) //MasterInfo& info : masters) {
        {
            //dbMaster* master = info.master();
            cout << "   - " << master->getName() << endl;
        }



    }


  


}


void
Netlist::technologyMapping() {

    int fiMax = fiDist_.xMax();
    vector<vector<Node*>> fi2CombNodes(fiMax+1);
    vector<vector<Node*>> fi2SequNodes(fiMax+1);

    //vector<vector<dbMaster*>> fi2CombMasters(fiMax+1);
    //vector<vector<dbMaster*>> fi2SequMasters(fiMax+1);
   
    //unordered_map<std::string, float> master2ratio;


    for(Node* node : nodes_) {
        if(node->getType() == NodeType::Combinational)
            fi2CombNodes[node->numFanins()].push_back(node);
        else if(node->getType() == NodeType::Sequential)
            fi2SequNodes[node->numFanins()].push_back(node);
        //fi2Nodes[node->numFanins()].push_back(node);
    }

    /*    
    vector<MasterInfo> masters = ang_->getMasterInfo();

    for(MasterInfo& info : masters) {
        dbMaster* master = info.master();
        float ratio = info.ratio();


        master2ratio[master->getName()] = ratio;

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
        //cout << master->getName() << " " 
        //     << inSigCnt << " " << outSigCnt << " " << inClkCnt << " " << outClkCnt << " " 
        //     << inRstCnt << " " << outRstCnt << " " << inScanCnt << " " << outScanCnt << endl;

        if(outSigCnt == 0)
            continue;
        
        if(inScanCnt > 0) {
            cout << "current version does not support scan-chain cell... (" << master->getName() << ")" <<  endl;
            continue;
        }

        if(inClkCnt > 0) {
            // sequential cell
            fi2SequMasters[inSigCnt].push_back(master);       
        } else {
            // combinational cell
            fi2CombMasters[inSigCnt].push_back(master);
        }
    }
    */
    
    // Control in
    //int clkIdx = nodeStor_.size();
    //int rstIdx = clkIdx+1;

    Node* clkIn = new Node();
    Node* rstIn = new Node();
    nodes_.push_back(clkIn);
    nodes_.push_back(rstIn);
    //nodeStor_.push_back(Node());
    //nodeStor_.push_back(Node());
    //nodes_.push_back(&nodeStor_[clkIdx]);
    //nodes_.push_back(&nodeStor_[rstIdx]);
    //Node* clkIn = nodes_[clkIdx];
    //Node* rstIn = nodes_[rstIdx];

    primIns_.push_back(clkIn);
    primIns_.push_back(rstIn);


    clkIn->setName("clk");
    clkIn->setType(NodeType::ClockIn);
    clkIn->setBin(getBin(0,0)); 
    rstIn->setName("rst");
    rstIn->setType(NodeType::ResetIn);
    rstIn->setBin(getBin(0,0)); 


    // Nangate Lef --> CLOCK 포트에대한 정의가 없음.
    // clock port이름을 input argument로 받고 
    // dbMTerm::getName() 과 비교하여 sequential MACRO를 찾아야 할듯. 
    // 혹은 lef파일 PIN-USE --> clock 추가
    int totNodeCnt=0;
    int beginIdx = 0;
    int endIdx = 0;
    float denominator;

    for(int fi = 0; fi <= fiMax; fi++) {
        denominator = 0.0;
        totNodeCnt = fi2SequNodes[fi].size();
        beginIdx = 0;

        unordered_map<dbMaster*, int> targetCnt;
        unordered_map<dbMaster*, int> currentCnt;


        for(dbMaster* master : fi2SequMasters_[fi]) {
            //denominator += master2ratio[master->getName()];
            denominator += master2ratio_[master];
            
            // ONLY_USE AT LEAST ONCE!
            //Node* target = fi2SequNodes[fi][beginIdx];
            //target->setDbMaster(master);
            //beginIdx++;
            //if(beginIdx >= totNodeCnt)
            //    break;
        }


        for(dbMaster* master : fi2SequMasters_[fi]) {
            //master2ratio[master->getName()] /= denominator;
            master2ratio_[master] /= denominator;
            //float ratio = master2ratio[master->getName()];
            float ratio = master2ratio_[master];
            //cout << "   " << master->getName() << " " << 100 * master2ratio_[master] << "\%" << endl;
            targetCnt[master] = ceil(ratio * totNodeCnt);
            currentCnt[master] = 0;
            //endIdx = beginIdx + ceil(ratio * totNodeCnt);
            //endIdx = min(endIdx, totNodeCnt);
            //for(int i=beginIdx; i < endIdx; i++) {
            //    Node* target = fi2SequNodes[fi][i];
            //    target->setDbMaster(master);
            //}
            //beginIdx = endIdx;
        }

        int numNodes = fi2SequNodes[fi].size();
        int numMasters = fi2SequMasters_[fi].size();
        int idxIter = 0;

        for(int i=0; i < numNodes; i++) {
            Node* target = fi2SequNodes[fi][i];
            dbMaster* master;
            
            while(true) {
                master = fi2SequMasters_[fi][idxIter];
                idxIter = (idxIter+1) % numMasters;
                if(targetCnt[master] - currentCnt[master] > 0) {
                    currentCnt[master]++;
                    break;
                }
                //if(targetCnt[master] > 0) {
                //    targetCnt[master]--;
                //    break;
                //}
            }
            target->setDbMaster(master);
        }
        
        // log
        cout << "# of inputs == " << fi << endl;
        cout << "<Sequantial>" << endl;
        for(dbMaster* master : fi2SequMasters_[fi]) {
            //printf("    %2.2f (%d / %d) - %s\n", master2ratio_[master], currentCnt[master], targetCnt[master], master->getName());
            printf("    %2.2f (%d/%d) - ", master2ratio_[master], currentCnt[master], targetCnt[master]);
            cout << master->getName() << endl; //, name);
        }



        //cout << "<Combinational>" << endl;
        denominator = 0.0;
        totNodeCnt = fi2CombNodes[fi].size();
        beginIdx = 0;
        targetCnt.clear();
        numNodes = fi2CombNodes[fi].size();
        numMasters = fi2CombMasters_[fi].size();
        idxIter = 0;



        for(dbMaster* master : fi2CombMasters_[fi]) {
            //denominator += master2ratio[master->getName()];
            denominator += master2ratio_[master];
            // ONLY_USE AT LEAST ONCE!
            //Node* target = fi2CombNodes[fi][beginIdx];
            //target->setDbMaster(master);
            //beginIdx++;
            //if(beginIdx >= totNodeCnt)
            //    break;
        }


        for(dbMaster* master : fi2CombMasters_[fi]) {
            master2ratio_[master] /= denominator;
            float ratio = master2ratio_[master];
            //cout << "   " << master->getName() << " " << 100 * master2ratio_[master] << "\%" << endl;
            targetCnt[master] = ceil(ratio * totNodeCnt);
            currentCnt[master] = 0;
            //endIdx = beginIdx + ceil(ratio * totNodeCnt);
            //endIdx = min(endIdx, totNodeCnt);
            //for(int i=beginIdx; i < endIdx; i++) {
            //    Node* target = fi2CombNodes[fi][i];
            //    target->setDbMaster(master);
            //}
            //beginIdx = endIdx;
        }

        for(int i=0; i < numNodes; i++) {
            Node* target = fi2CombNodes[fi][i];
            dbMaster* master;
            while(true) {
                master = fi2CombMasters_[fi][idxIter];
                idxIter = (idxIter+1) % numMasters;
                //if(targetCnt[master] > 0) {
                //    targetCnt[master]--;
                //    break;
                //}
                if(targetCnt[master] - currentCnt[master] > 0) {
                    currentCnt[master]++;
                    break;
                }
            }
            target->setDbMaster(master);
        }

        cout << "<Combinational>" << endl;
        for(dbMaster* master : fi2CombMasters_[fi]) {
            string name = master->getName();
            printf("    %2.2f (%d/%d) - ", master2ratio_[master], currentCnt[master], targetCnt[master]);
            cout << master->getName() << endl; //, name);
        }
    }


    for(Node* node : nodes_) { 
        node->mappingTerms(clkIn, rstIn); 
    }
    ///////////////////
    int netCnt=0;
    nets_.reserve(2* ang_->getInstanceCnt());
    unordered_map<string, Net> name2net;
    unordered_map<string, string> out2name;
    
    for(int i=0; i < nodes_.size(); i++) {
        Node* source = nodes_[i];
        unordered_map<string, vector<Node*>> out2sinks;

        for(Node* sink : source->getSinks()) {
            string outTermName = source->getTerm(sink);
            out2sinks[outTermName].push_back(sink);
        }

        string netName = "";
        // check whether output term is connected to IO
        for(auto p : out2sinks) {
            string outTermName = p.first;
            vector<Node*> sinks = p.second;

            if( source->getType() == NodeType::PrimaryIn || 
                source->getType() == NodeType::ClockIn ||
                source->getType() == NodeType::ResetIn ) {
                netName = source->getName();
            } else {
                bool containPrimOut = false;
                netName = "net" + to_string(netCnt);
                for(Node* sink : sinks) {
                    if(sink->getType()==NodeType::PrimaryOut) {
                        netName = sink->getName();
                        containPrimOut = true;
                    }
                }

                if(!containPrimOut)
                    netCnt++;
            }

            out2name[outTermName] = netName;

            if(name2net.find(netName) == name2net.end()) {
                name2net[netName] = Net();
                name2net[netName].setName(netName);
                name2net[netName].addTerm(source, outTermName);
            }

            for(Node* sink : sinks) {
                string inTermName = sink->getTerm(source);
                name2net[netName].addTerm(sink, inTermName);
            }
        
        }


        for(auto p : out2sinks) {
            string outTermName = p.first;
            string netName = out2name[outTermName];
            vector<Node*> sinks = p.second;
        }

        /*
        for(Node* sink : source->getSinks()) {
            string outTermName = source->getTerm(sink);
            string inTermName = sink->getTerm(source);
            
            if(out2net.find(outTermName) == out2net.end()) {
                switch(source->getType()) {
                    case NodeType::PrimaryIn:
                        out2net[outTermName] = source->getName(); break;
                        //netName = source->getName();
                    case NodeType::ResetIn:
                        out2net[outTermName] = source->getName(); break;
                        //netName = source->getName();
                    case NodeType::ClockIn:
                        out2net[outTermName] = source->getName(); break; //netName = source->getName();
                    default:
                        out2net[outTermName] = "net"+to_string(netCnt++); break;
                        //netName = "net"+to_string(netCnt++);
                }
            }

            netName = out2net[outTermName];

            if(src2net.find(netName) == src2net.end()) {
                src2net[netName] = Net();
                src2net[netName].setName(netName);
                src2net[netName].addTerm( source, outTermName );
            }

            Net* net = &src2net[netName];
            net->addTerm( sink, inTermName );
        }
        */
    }

    for(auto it = name2net.begin(); it != name2net.end(); it++) {
        netStor_.push_back(it->second);
        //nets_.push_back(&netStor_.back());
        Net* net = &netStor_.back(); //&it->second;
        //net->print();
    }

    cout << "technology mapping is finished" << endl;





    for(int i=0; i < netStor_.size(); i++) {
        Net* net = &netStor_[i];
        nets_.push_back(net);
        //nets_[i]->print();
    }
        //netStor_[i].print(); //nets_[i]->print();
    //for(Net* net : nets_)
    //    net->print();


    /*
    for(int i=0; i < nodes_.size(); i++) {
        Node* node = nodes_[i];

        if(node->getType() == NodeType::PrimaryIn || node->getType() == NodeType::PrimaryOut)
            continue;

        
        dbMaster* master = node2master[node];

        cout << i << "-th node is mapped to " << master->getName() << endl;

    }
    */
}


int Netlist::getMaxTopologicalOrder() {
//cout << "1" << endl;
    int maxTopoOrder = 0;
    unordered_map<Node*, int> topoOrder = topologicalSort();
//cout << "2" << endl;
    for(Node* node : nodes_) {
        maxTopoOrder = max(maxTopoOrder, topoOrder[node]);
    }
//cout << "3" << endl;
    return maxTopoOrder;
}

double Netlist::getAvgTopologicalOrder() {
    double avgTopoOrder = 0;
    unordered_map<Node*, int> topoOrder = topologicalSort();
    for(Node* node : nodes_) {
        avgTopoOrder += topoOrder[node];
    }
    avgTopoOrder /= nodes_.size();
    return avgTopoOrder;
}


unordered_map<Node*, int> Netlist::topologicalSort() {

    queue<Node*> Q;

    unordered_map<Node*, int> inDegree;
    unordered_map<Node*, int> topoOrder;

    for(int i=0; i < nodes_.size(); i++) {
        Node* node = nodes_[i];
        
        topoOrder[node] = -1; //.insert(make_pair(node, -1));

        switch(node->getType()) {
            case NodeType::Sequential:
                inDegree[node] = 0; break; //.insert(make_pair(node, 0)); break;
            case NodeType::PrimaryIn:
                inDegree[node] = 0; break; //inDegree.insert(make_pair(node, 0)); break;
            default:
                inDegree[node] = node->numFanins(); break; 
                //inDegree.insert(make_pair(node, node->numFanins())); break;
        }

        if(inDegree[node] == 0) {
            topoOrder[node] = 0;
            if(node->getType() == NodeType::ClockIn || node->getType() == NodeType::ResetIn)
                continue;
            Q.push(node);
        }
    }

    while(!Q.empty()) {
        Node* n1 = Q.front();
        Q.pop();

        if(n1->getType() == NodeType::ClockIn || n1->getType() == NodeType::ResetIn)
        {
            cout << "???????" << endl;
            exit(0);
        }


        for(Node* n2 : n1->getSinks()) {

            if( inDegree[n2] == 0 ) continue;

            if( --inDegree[n2] == 0 ) {
                topoOrder[n2] = topoOrder[n1] + 1;
                Q.push(n2);
            }
        }
    }

    return topoOrder;
}

int
Netlist::getCountNodeType(int type) {
    int count =0;
    for(Node* node : nodes_)
        if(node->getType() == type) count++;

    return count;
}


double
Netlist::getCombinationalRatio() {
    int numCombNodes = 0;
    int numSequNodes = 0;

    for(Node* node : nodes_) {
        if(node->getType() == NodeType::Sequential)
            numSequNodes++;
        else if(node->getType() == NodeType::Combinational)
            numCombNodes++;
    }

    double currentRatio = 1.0 * numCombNodes / (numSequNodes + numCombNodes);

    return currentRatio;
}



void
Netlist::timingPathConstruction_v1() {
    // TODO
    //double synClkPeriod = ang_->getSynClkPeriod();
    double avgGateDelay = ang_->getAvgGateDelay();
    double avgTopoOrder = ang_->getAvgTopoOrder();


    int targetMaxOrder = ceil(avgTopoOrder) + 2; //ceil( synClkPeriod / avgGateDelay );
    //cout << "timing path construction (target clk : " << synClkPeriod << " avg. gate delay : " << avgGateDelay << ")" << endl;
    cout << "timing path construction (target avg. topo. order : " << avgTopoOrder << ")" << endl;

    int numIter = 0;

    while(true) {
        unordered_map<Node*, int> topoOrder = topologicalSort();
        bool updated = false;
        for(Node* n : nodes_) {
            if(topoOrder[n] == targetMaxOrder) {
                if(n->getType() == NodeType::Combinational) {
                    n->setType(NodeType::Sequential);
                    updated = true;
                }
            }
        }
  
    
        if(numIter++ % 5 == 0) {
            cout << numIter++ << "-iteration target " << avgTopoOrder << " cur_avg " << getAvgTopologicalOrder() << " cur_max " << getMaxTopologicalOrder() << endl;
        }

        if(!updated)
            break;
    }


    // TODO 
    // fit to comb_ratio (input arg)
    int numTotNodes = nodes_.size();
    int numCombNodes = 0;
    int numSequNodes = 0;
    vector<vector<Node*>> targets(layoutDimX_ * layoutDimY_);
    vector<int> idx;

    for(int i=0; i < layoutDimX_ * layoutDimY_; i++)
        idx.push_back(i);

    for(Node* node : nodes_) {
        if(node->getType() == NodeType::Sequential) {
            int x = node->x();
            int y = node->y();
            int id = x + y * layoutDimX_;
            targets[id].push_back(node);
            numSequNodes++;
        } else if (node->getType() == NodeType::Combinational) {
            numCombNodes++;
        }

    }
    


    double currentRatio = 1.0 * numCombNodes / (numCombNodes + numSequNodes);
    
    cout << "Target combination ratio   : " << ang_->getCombRatio() << endl;
    cout << "Current combination ratio  : " << currentRatio << endl;
    cout << "Current max topo order     : " << getMaxTopologicalOrder() << endl;
    cout << "Current avg topo order     : " << getAvgTopologicalOrder() << endl;
    int edgeLength = 0;
    bool finish = false;


    //for(int edgeLength=0; edgeLength < layoutDimX_+layoutDimY_; edgeLength++) {
    //for(int edgeLength=0; edgeLength < 3; edgeLength++) {
    int maxLen = ceil(0.2 * (layoutDimX_ + layoutDimY_));
    cout << "Max len : " << maxLen << endl;
    for(int edgeLength=0; edgeLength < max(3, maxLen); edgeLength++) {

        if(currentRatio >= ang_->getCombRatio())
            break;
        random_shuffle(idx.begin(), idx.end());
        for(int id : idx) {
            int cx = id % layoutDimX_;
            int cy = id / layoutDimX_;
            int lx = max(cx - edgeLength, 0);
            int ly = max(cy - edgeLength, 0);
            int ux = min(cx + edgeLength, layoutDimX_-1);
            int uy = min(cy + edgeLength, layoutDimY_-1);

            vector<Node*> candidates;

            for(int x=lx; x<=ux; x++) {
                for(int y=ly; y<=uy; y++) {
                    int tIdx = x + y*layoutDimX_;
                    candidates.insert(candidates.end(), targets[tIdx].begin(), targets[tIdx].end());
                }
            }


            
            if(candidates.size() < 2)
                continue;
            else {
                //sort(candidates.begin(), candidates.end(), [](Node* left, Node* right) {
                //    return left->numFanouts() < right->numFanouts();
                //        });
                sort(candidates.begin(), candidates.end(), [](Node* left, Node* right) {
                    return left->numFanins() < right->numFanins();
                        });


                bool found=false;
                Node *n1, *n2;

                for(int i=0; i < candidates.size()-1; i++) {
                    n1 = candidates[i];
                    n2 = candidates[i+1];
                    int totFanins = n1->numFanins() + n2->numFanins();
                    int totFanouts = n1->numFanouts() + n2->numFanouts();

                    if(totFanins <= fiDist_.xMax() && totFanouts < foDist_.xMax()) {
                        found=true;
                        break;
                    } 
                }

                if(!found) continue;

                Node* n3 = createMergeNode(n1, n2, true);
                n1->setType(NodeType::Combinational);
                n2->setType(NodeType::Combinational);

                int idx1 = n1->x() + n1->y() * layoutDimX_;
                int idx2 = n2->x() + n2->y() * layoutDimX_;
                int idx3 = n3->x() + n3->y() * layoutDimX_;
                targets[idx1].erase(find(targets[idx1].begin(), targets[idx1].end(), n1));
                targets[idx2].erase(find(targets[idx2].begin(), targets[idx2].end(), n2));
                targets[idx3].push_back(n3);

                //numTotNodes++;
                numCombNodes += 2;
                numSequNodes -= 1; 
                //currentRatio = 1.0 * (numTotNodes - numSequNodes) / numTotNodes;
                //currentRatio = 1.0 * numCombNodes / (numCombNodes + numSequNodes);
                currentRatio = 1.0 * (numCombNodes+numSequNodes) / (numCombNodes + 2*numSequNodes); 
                //cout << "current ratio : " << currentRatio << "(" << getCombinationalRatio() << ")" << endl;
            }

            if(currentRatio >= ang_->getCombRatio())
                break;
        }


    }

    for(int edgeLength=0; edgeLength < max(3, maxLen); edgeLength++) {

        if(currentRatio >= ang_->getCombRatio())
            break;
        
        random_shuffle(idx.begin(), idx.end());
        for(int id : idx) {
            int cx = id % layoutDimX_;
            int cy = id / layoutDimX_;
            int lx = max(cx - edgeLength, 0);
            int ly = max(cy - edgeLength, 0);
            int ux = min(cx + edgeLength, layoutDimX_-1);
            int uy = min(cy + edgeLength, layoutDimY_-1);

            vector<Node*> candidates;

            for(int x=lx; x<=ux; x++) {
                for(int y=ly; y<=uy; y++) {
                    int tIdx = x + y*layoutDimX_;
                    candidates.insert(candidates.end(), targets[tIdx].begin(), targets[tIdx].end());
                }
            }

            if(candidates.size() < 2)
                continue;
            else {
                sort(candidates.begin(), candidates.end(), [](Node* left, Node* right) {
                    return left->numFanouts() < right->numFanouts();
                        });

                bool found=false;
                Node *n1, *n2, *n3;

                for(int i=0; i < candidates.size()-1; i++) {
                    n1 = candidates[i];
                    n2 = candidates[i+1];
                    int totFanouts = n1->numFanouts() + n2->numFanouts();
                    if(totFanouts < foDist_.xMax()) {
                        found=true;
                    }
                    break;
                }
               
                if(found) {
                    Node* n3 = createMergeNode(n1, n2, false);
                    n1->setType(NodeType::Combinational);
                    n2->setType(NodeType::Combinational);

                    int idx1 = n1->x() + n1->y() * layoutDimX_;
                    int idx2 = n2->x() + n2->y() * layoutDimX_;
                    int idx3 = n3->x() + n3->y() * layoutDimX_;
                    targets[idx1].erase(find(targets[idx1].begin(), targets[idx1].end(), n1));
                    targets[idx2].erase(find(targets[idx2].begin(), targets[idx2].end(), n2));
                    targets[idx3].push_back(n3);

                    numCombNodes += 2;
                    numSequNodes -= 1;
                    currentRatio = 1.0 * (numCombNodes+numSequNodes) / (numCombNodes + 2*numSequNodes); 
                } 
            }

            if(currentRatio >= ang_->getCombRatio())
                break;
        }


        if(currentRatio >= ang_->getCombRatio())
            break;
    }

    cout << "Current max topo order     : " << getMaxTopologicalOrder() << endl;
    cout << "Current comb_ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl;
    cout << getCombinationalRatio() << endl;

    cout << "#comb : " << getCountNodeType(NodeType::Combinational) << endl;
    cout << "#sequ : " << getCountNodeType(NodeType::Sequential) << endl;

    for(int i=0; i < nodes_.size(); i++) {
        Node* node = nodes_[i];
        if(node->getType() == NodeType::Sequential) {
            //if(node->numFanins() > 1) {
            //    node->setType(NodeType::Combinational);
            //    insertSequentialNode(node, false);
            //}
            if(!hasMaster(node->numFanins(), true)) {
                node->setType(NodeType::Combinational);
                insertSequentialNode(node, false);
            }
        }
    }

    numCombNodes = 0;
    numSequNodes = 0;

    for(Node* node : nodes_) {
        if(node->getType() == NodeType::Sequential)
            numSequNodes++;
        else if(node->getType() == NodeType::Combinational)
            numCombNodes++;
    }

    currentRatio = 1.0 * numCombNodes / (numSequNodes + numCombNodes);

    cout << "Current comb_ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl;
    cout << getCombinationalRatio() << endl;
    cout << "#comb : " << getCountNodeType(NodeType::Combinational) << endl;
    cout << "#sequ : " << getCountNodeType(NodeType::Sequential) << endl;
    cout << "Max topological order : " << getMaxTopologicalOrder() << " (target max order -> " << targetMaxOrder << ")" << endl;

}




void
Netlist::timingPathConstruction_v2() {
    // TODO
    //double synClkPeriod = ang_->getSynClkPeriod();
    double avgGateDelay = ang_->getAvgGateDelay();
    double avgTopoOrder = ang_->getAvgTopoOrder();


    int targetMaxOrder = ceil(avgTopoOrder) + 2; //ceil( synClkPeriod / avgGateDelay );
    //cout << "timing path construction (target clk : " << synClkPeriod << " avg. gate delay : " << avgGateDelay << ")" << endl;
    cout << "timing path construction (target avg. topo. order : " << avgTopoOrder << ")" << endl;

    int numIter = 0;

    while(true) {
        unordered_map<Node*, int> topoOrder = topologicalSort();
        bool updated = false;
        for(Node* n : nodes_) {
            if(topoOrder[n] == targetMaxOrder) {
                if(n->getType() == NodeType::Combinational) {
                    n->setType(NodeType::Sequential);
                    updated = true;
                }
            }
        }
  
     
        cout << numIter++ << "-iteration target " << avgTopoOrder << " cur_avg " << getAvgTopologicalOrder() << " cur_max " << getMaxTopologicalOrder() << endl;


        if(!updated)
            break;
    }


    // TODO 
    // fit to comb_ratio (input arg)
    int numTotNodes = nodes_.size();
    int numCombNodes = 0;
    int numSequNodes = 0;
   


    double currentRatio = 1.0 * numCombNodes / (numCombNodes + numSequNodes);
    
    cout << "Target combination ratio   : " << ang_->getCombRatio() << endl;
    cout << "Current combination ratio  : " << currentRatio << endl;
    cout << "Current max topo order     : " << getMaxTopologicalOrder() << endl;
    cout << "Current avg topo order     : " << getAvgTopologicalOrder() << endl;
    int edgeLength = 0;
    bool finish = false;

    vector<int> idx;
    for(int i=0; i < layoutDimX_ * layoutDimY_; i++)
        idx.push_back(i);

    //for(int edgeLength=0; edgeLength < layoutDimX_+layoutDimY_; edgeLength++) {
    //for(int edgeLength=0; edgeLength < 3; edgeLength++) {
    int maxLen = ceil(0.5 * (layoutDimX_ + layoutDimY_));
    cout << "Max len : " << maxLen << endl;
    for(int edgeLength=0; edgeLength < max(3, maxLen); edgeLength++) {

        if(currentRatio >= ang_->getCombRatio())
            break;
        
        vector<vector<Node*>> targets(layoutDimX_ * layoutDimY_);

        for(Node* node : nodes_) {
            if(node->getType() == NodeType::Sequential) {
                int x = node->x();
                int y = node->y();
                int id = x + y * layoutDimX_;
                targets[id].push_back(node);
                numSequNodes++;
            } else if (node->getType() == NodeType::Combinational) {
                numCombNodes++;
            }

        }
        
        random_shuffle(idx.begin(), idx.end());
        for(int id : idx) {
            int cx = id % layoutDimX_;
            int cy = id / layoutDimX_;
            int lx = max(cx - edgeLength, 0);
            int ly = max(cy - edgeLength, 0);
            int ux = min(cx + edgeLength, layoutDimX_-1);
            int uy = min(cy + edgeLength, layoutDimY_-1);

            vector<Node*> candidates;
            for(int x=lx; x<=ux; x++) {
                for(int y=ly; y<=uy; y++) {
                    int tIdx = x + y*layoutDimX_;
                    candidates.insert(candidates.end(), targets[tIdx].begin(), targets[tIdx].end());
                }
            }


            if(candidates.size() < 2)
                continue;
            
            bool isDone = false;
            Node *n1, *n2, *n3;

            vector<Node*> faninSorted = candidates;
            sort(faninSorted.begin(), faninSorted.end(), [](Node* left, Node* right) {
                return left->numFanins() < right->numFanins();
                    });
            
            int i=0;
            while(i<faninSorted.size()-1) {
                n1 = faninSorted[i];
                n2 = faninSorted[i+1];
                int totFanins = n1->numFanins() + n2->numFanins();
                //cout << "# fanins to merge = " << totFanins << endl;
                if(totFanins <= fiDist_.xMax()) {
                    n3 = createMergeNode(n1,n2,true);
                    //cout << "merged!" << endl;
                    n1->setType(NodeType::Combinational);
                    n2->setType(NodeType::Combinational);

                    int idx1 = n1->x() + n1->y() * layoutDimX_;
                    int idx2 = n2->x() + n2->y() * layoutDimX_;
                    int idx3 = n3->x() + n3->y() * layoutDimX_;
                    candidates.erase(find(candidates.begin(), candidates.end(), n1));
                    candidates.erase(find(candidates.begin(), candidates.end(), n2));
                    candidates.push_back(n3);


                    currentRatio = getCombinationalRatio();
                    //cout << "current ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl; 
                    
                    i += 2;
                    if(currentRatio >= ang_->getCombRatio())
                        break;
                
                } else {
                    break;
                }
            }
                
            if(candidates.size() < 2)
                continue;

            vector<Node*> fanoutSorted = candidates;
            sort(fanoutSorted.begin(), fanoutSorted.end(), [](Node* left, Node* right) {
                return left->numFanouts() < right->numFanouts();
                    });
            i=0;
            while(i<fanoutSorted.size()-1) {
                n1 = fanoutSorted[i];
                n2 = fanoutSorted[i+1];
                int totFanouts = n1->numFanouts() + n2->numFanouts();
                //cout << "# fanouts to merge = " << totFanouts << endl;
                if(totFanouts <= foDist_.xMax()) {
                    n3 = createMergeNode(n1,n2,false);
                    //cout << "merged!" << endl;
                    n1->setType(NodeType::Combinational);
                    n2->setType(NodeType::Combinational);

                    int idx1 = n1->x() + n1->y() * layoutDimX_;
                    int idx2 = n2->x() + n2->y() * layoutDimX_;
                    int idx3 = n3->x() + n3->y() * layoutDimX_;

                    

                    candidates.erase(find(candidates.begin(), candidates.end(), n1));
                    candidates.erase(find(candidates.begin(), candidates.end(), n2));
                    candidates.push_back(n3);
                    currentRatio = getCombinationalRatio();
                    //cout << "current ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl; 
                    i+=2;
                    if(currentRatio >= ang_->getCombRatio())
                        break;
                } else {
                    break;
                }

            }

            if(currentRatio >= ang_->getCombRatio())
                break;
        }
    }



    //for(int edgeLength=0; edgeLength < layoutDimX_+layoutDimY_; edgeLength++) {
    //for(int edgeLength=0; edgeLength < 3; edgeLength++) {
    //for(int edgeLength=0; edgeLength < max(3, maxLen); edgeLength++) {

    //    if(currentRatio >= ang_->getCombRatio())
    //        break;
    //    
    //    random_shuffle(idx.begin(), idx.end());
    //    for(int id : idx) {
    //        int cx = id % layoutDimX_;
    //        int cy = id / layoutDimX_;
    //        int lx = max(cx - edgeLength, 0);
    //        int ly = max(cy - edgeLength, 0);
    //        int ux = min(cx + edgeLength, layoutDimX_-1);
    //        int uy = min(cy + edgeLength, layoutDimY_-1);
    //        vector<Node*> candidates;
    //        for(int x=lx; x<=ux; x++) {
    //            for(int y=ly; y<=uy; y++) {
    //                int tIdx = x + y*layoutDimX_;
    //                candidates.insert(candidates.end(), targets[tIdx].begin(), targets[tIdx].end());
    //            }
    //        }
    //        if(candidates.size() < 2)
    //            continue;
    //        else {
    //            sort(candidates.begin(), candidates.end(), [](Node* left, Node* right) {
    //                return left->numFanouts() < right->numFanouts();
    //                    });
    //            bool insertFront =false;
    //            Node* n1 = candidates[0];
    //            Node* n2 = candidates[1];
    //            Node* n3 = candidates[2];
    //            int totFanins = n1->numFanins() + n2->numFanins() + n3->numFanins();
    //            int totFanouts = n1->numFanouts() + n2->numFanouts() + n3->numFanouts();
    //            if(totFanins <= fiDist_.xMax()) {
    //                insertFront = true;
    //            } else if(totFanouts <= foDist_.xMax()) {
    //                insertFront = false;
    //            } else {
    //                continue;
    //            }
    //            Node* n3 = createMergeNode(n1, n2, insertFront);
    //            bool found=false;
    //            Node *n1, *n2, *n3;

    //            for(int i=0; i < candidates.size()-1; i++) {
    //                n1 = candidates[i];
    //                n2 = candidates[i+1];
    //                int totFanouts = n1->numFanouts() + n2->numFanouts();
    //                if(totFanouts < foDist_.xMax()) {
    //                    found=true;
    //                }
    //                break;
    //            }
    //           
    //            if(found) {
    //                Node* n3 = createMergeNode(n1, n2, false);
    //                n1->setType(NodeType::Combinational);
    //                n2->setType(NodeType::Combinational);

    //                int idx1 = n1->x() + n1->y() * layoutDimX_;
    //                int idx2 = n2->x() + n2->y() * layoutDimX_;
    //                int idx3 = n3->x() + n3->y() * layoutDimX_;
    //                targets[idx1].erase(find(targets[idx1].begin(), targets[idx1].end(), n1));
    //                targets[idx2].erase(find(targets[idx2].begin(), targets[idx2].end(), n2));
    //                targets[idx3].push_back(n3);

    //                numCombNodes += 2;
    //                numSequNodes -= 1;
    //                //currentRatio = 1.0 * numCombNodes / (numCombNodes + numSequNodes); //(numTotNodes - numSequNodes) / numTotNodes;
    //                currentRatio = 1.0 * (numCombNodes+numSequNodes) / (numCombNodes + 2*numSequNodes); 
    //                //cout << "current ratio : " << currentRatio << endl;
    //                //cout << "current ratio : " << currentRatio << "(" << getCombinationalRatio() << ")" << endl;
    //            } 
    //        }
    //        if(currentRatio >= ang_->getCombRatio())
    //            break;
    //    }
    //    if(currentRatio >= ang_->getCombRatio())
    //        break;
    //}


    /*
    while(true) {
        unordered_map<Node*, int> topoOrder = topologicalSort();
        bool updated = false;
        for(Node* n : nodes_) {
            if(topoOrder[n] == targetMaxOrder) {
                if(n->getType() == NodeType::Combinational) {
                    n->setType(NodeType::Sequential);
                    updated = true;
                }
            }
        }
   
        if(!updated)
            break;
    }
    */

    cout << "Current max topo order     : " << getMaxTopologicalOrder() << endl;
    cout << "Current comb_ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl;
    cout << getCombinationalRatio() << endl;

    cout << "#comb : " << getCountNodeType(NodeType::Combinational) << endl;
    cout << "#sequ : " << getCountNodeType(NodeType::Sequential) << endl;

    cout << "HERE1" << endl;
    for(int i=0; i < nodes_.size(); i++) {
        Node* node = nodes_[i];
        if(node->getType() == NodeType::Sequential) {
            //if(node->numFanins() > 1) {
            //    node->setType(NodeType::Combinational);
            //    insertSequentialNode(node, false);
            //}
            if(!hasMaster(node->numFanins(), true)) {
                //cout << node->getName() << " " << node->numFanins() << endl;
                node->setType(NodeType::Combinational);
                insertSequentialNode(node, false);
            }
        }
    }
    cout << "HERE2" << endl;

    numCombNodes = 0;
    numSequNodes = 0;

    for(Node* node : nodes_) {
        if(node->getType() == NodeType::Sequential)
            numSequNodes++;
        else if(node->getType() == NodeType::Combinational)
            numCombNodes++;
    }

    currentRatio = 1.0 * numCombNodes / (numSequNodes + numCombNodes);

    cout << "Current comb_ratio : " << currentRatio << " (" << ang_->getCombRatio() << ")" << endl;
    cout << getCombinationalRatio() << endl;
    cout << "#comb : " << getCountNodeType(NodeType::Combinational) << endl;
    cout << "#sequ : " << getCountNodeType(NodeType::Sequential) << endl;
    //for(Node* node : nodes_) {
    //for(auto& candidates : targets) {
    //    for(Node* node : candidates) {
    //        if(node->getType() == NodeType::Sequential) {
    //            if(node->numFanins() > 1) {
    //                node->setType(NodeType::Combinational);
    //                insertSequentialNode(node, false);
    //            }
    //        }
    //    }
    //}



    //for(auto& candidates : targets) {
    //    for(Node* target: candidates) {
    //        target->setType(NodeType::Combinational);
    //        insertSequentialNode(target, false);
    //    }
    //}


    cout << "Max topological order : " << getMaxTopologicalOrder() << " (target max order -> " << targetMaxOrder << ")" << endl;


    /*

    while(currentRatio < ang_->getCombRatio()) {

        int minDist = INT_MAX;
        Node *n1, *n2, *n3;
        for(int i=0; i < targets.size()-1; i++) {
            Node* candi1 = targets[i];
            for(int j=i+1; j < targets.size(); j++) {
                Node* candi2 = targets[j];

                int totFanouts = candi1->numFanouts() + candi2->numFanouts();

                if(totFanouts > foDist_.xMax())
                    continue;

                int curDist = abs(candi1->x() - candi2->x()) + abs(candi1->y() - candi2->y());
                


                if(minDist > curDist) {
                    minDist = curDist;
                    n1 = candi1;
                    n2 = candi2;
                }
            }
        }


        //Node* n1 = targets[0];
        //Node* n2 = targets[1];
        n3 = createMergeNode(n1, n2);
        n1->setType(NodeType::Combinational);
        n2->setType(NodeType::Combinational);


        targets.erase(find(targets.begin(), targets.end(), n1));
        targets.erase(find(targets.begin(), targets.end(), n2));

        targets.push_back(n3);
        currentRatio = 1.0 * (nodes_.size() - targets.size()) / nodes_.size();

    }

    // TODO
    // insert flipflops

    for(Node* target : targets) {
        target->setType(NodeType::Combinational);
        insertSequentialNode(target, false);
    }
    */
}


bool Netlist::hasMaster(int numFanins, bool isSequential) {
    if(isSequential) 
        return fi2SequMasters_[numFanins].size() > 0 ? true : false;
    else
        return fi2CombMasters_[numFanins].size() > 0 ? true : false;

}

Node* Netlist::createMergeNode(Node* n1, Node* n2, bool front) {

    string cellName = "merge" + to_string(nodes_.size());
    

    Node* n3 = new Node();
    nodes_.push_back(n3);
    //nodeStor_.push_back(Node());
    //nodes_.push_back(&nodeStor_.back());
    //Node* n3 = nodes_.back();
    n3->setType(NodeType::Sequential);
    n3->setBin(n1->getBin());
    
    int lx = min(n1->lx(), n2->lx());
    int ly = min(n1->ly(), n2->ly());
    int ux = max(n1->ux(), n2->ux());
    int uy = max(n1->uy(), n2->uy());

    n3->setBbox(lx, ly, ux, uy);
    n3->setName(cellName);
    n3->getBin()->addNode(n3);

    if(front) {
        vector<Node*> sources;
        vector<Node*>::iterator it;

        for(Node* node: n1->getSources()) {
            it = find(sources.begin(), sources.end(), node);
            if(it == sources.end()) {
                sources.push_back(node);
            }
        }

        for(Node* node: n2->getSources()) {
            it = find(sources.begin(), sources.end(), node);
            if(it == sources.end()) {
                sources.push_back(node);
            }
        }


        n3->addSources(sources);
        n3->addSink(n1);
        n3->addSink(n2);
        n1->removeAllSources();
        n2->removeAllSources();
        n1->addSource(n3);
        n2->addSource(n3);


    } else {
        vector<Node*> sinks;
        vector<Node*>::iterator it;

        for(Node* node : n1->getSinks()) {
            it = find(sinks.begin(), sinks.end(), node);
            if(it == sinks.end()) {
                sinks.push_back(node);
                //node->addSource(n3);
            }
        }

        for(Node* node : n2->getSinks()) {
            it = find(sinks.begin(), sinks.end(), node);
            if(it == sinks.end()) {
                sinks.push_back(node);
                //node->addSource(n3);
            }
        }

        n3->addSinks(sinks);
        n3->addSource(n1);
        n3->addSource(n2);
        n1->removeAllSinks();
        n2->removeAllSinks();
        n1->addSink(n3);
        n2->addSink(n3);
    }
    n1->getBin()->update(n1);
    n2->getBin()->update(n2);
    n3->getBin()->update(n3);

    return n3;
}

Node* Netlist::createMergeNode(Node* n1, Node* n2, Node* n3, bool front) {
    string cellName = "merge" + to_string(nodes_.size());
 

    int centerX = (n1->x() + n2->x() + n3->x())/3;
    int centerY = (n1->y() + n2->y() + n3->y())/3;


    Node* n4 = new Node();
    nodes_.push_back(n4);
    //nodeStor_.push_back(Node());
    //nodes_.push_back(&nodeStor_.back());
    //Node* n4 = nodes_.back();

    n4->setType(NodeType::Sequential);
    n4->setBin(getBin(centerX, centerY)); 
    
    int lx = min(min(n1->lx(), n2->lx()), n3->lx());
    int ly = min(min(n1->ly(), n2->ly()), n3->ly());
    int ux = max(max(n1->ux(), n2->ux()), n3->ux());
    int uy = max(max(n1->uy(), n2->uy()), n3->uy());

    n4->setBbox(lx, ly, ux, uy);
    n4->setName(cellName);
    n4->getBin()->addNode(n4);

    if(front) {
        vector<Node*> sources;
        vector<Node*>::iterator it;

        for(Node* node: n1->getSources()) {
            it = find(sources.begin(), sources.end(), node);
            if(it == sources.end()) {
                sources.push_back(node);
            }
        }

        for(Node* node: n2->getSources()) {
            it = find(sources.begin(), sources.end(), node);
            if(it == sources.end()) {
                sources.push_back(node);
            }
        }

        for(Node* node: n3->getSources()) {
            it = find(sources.begin(), sources.end(), node);
            if(it == sources.end()) {
                sources.push_back(node);
            }
        }
        n4->addSources(sources);
        n4->addSink(n1);
        n4->addSink(n2);
        n4->addSink(n3);
        n1->removeAllSources();
        n2->removeAllSources();
        n3->removeAllSources();
        n1->addSource(n4);
        n2->addSource(n4);
        n3->addSource(n4);


    } else {

        vector<Node*> sinks;
        vector<Node*>::iterator it;

        for(Node* node : n1->getSinks()) {
            it = find(sinks.begin(), sinks.end(), node);
            if(it == sinks.end()) {
                sinks.push_back(node);
            }
        }

        for(Node* node : n2->getSinks()) {
            it = find(sinks.begin(), sinks.end(), node);
            if(it == sinks.end()) {
                sinks.push_back(node);
            }
        }

        for(Node* node : n3->getSinks()) {
            it = find(sinks.begin(), sinks.end(), node);
            if(it == sinks.end()) {
                sinks.push_back(node);
            }
        }


        n4->addSinks(sinks);
        n4->addSource(n1);
        n4->addSource(n2);
        n4->addSource(n3);
        n1->removeAllSinks();
        n2->removeAllSinks();
        n3->removeAllSinks();
        n1->addSink(n4);
        n2->addSink(n4);
        n3->addSink(n4);
    }
    n1->getBin()->update(n1);
    n2->getBin()->update(n2);
    n3->getBin()->update(n3);
    n4->getBin()->update(n4);

    return n4;
}


void
Netlist::insertSequentialNode(Node* target, bool front) {

    string cellName = "s" + to_string(nodes_.size());
    Node* seqn = new Node();
    nodes_.push_back(seqn);
    
    //Node n1;
    //nodeStor_.push_back(n1);
    //nodes_.push_back(&nodeStor_.back()); 
    //Node* seqn = nodes_.back();
    Bin* bin = target->getBin();
    seqn->setType(NodeType::Sequential);
    seqn->setBin(target->getBin());
    seqn->setBbox(target->x(), target->y(), target->x(), target->y());
    seqn->setName(cellName);
    bin->addNode(seqn);
    if(front) {
        vector<Node*> sources = target->getSources();
        if(sources.size() > 1) {
            Node* combn = new Node();
            nodes_.push_back(combn);
            //Node n2;
            //nodeStor_.push_back(n2);
            //nodes_.push_back(&nodeStor_.back()); 
            //Node* combn = nodes_.back(); //nodeStor_.back();
            combn->setType(NodeType::Combinational);
            combn->setBin(target->getBin());
            combn->setBbox(target->x(), target->y(), target->x(), target->y());
            combn->addSources(sources);
            combn->addSink(seqn);
            seqn->addSource(combn);
            target->getBin()->addNode(combn);
        } else {
           seqn->addSources(sources);
        }
        seqn->addSink(target);
        target->removeAllSources();
        target->addSource(seqn);
    } else {
        vector<Node*> sinks = target->getSinks();
        seqn->addSinks(sinks);
        seqn->addSource(target);
        target->removeAllSinks();
        target->addSink(seqn);
    }
    seqn->getBin()->update(seqn);
    target->getBin()->update(target);
}





};


