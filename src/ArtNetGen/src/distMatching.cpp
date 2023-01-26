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
Netlist::distMatching() {

    int tarEdgeCnt = edgeDist_.totalCnt();
    int curEdgeCnt = 0;


    vector<Bin*> srcBins = bins_;
    vector<int> edgeSampling = edgeDist_.getSamplingVector();


    cout << "Total # of edges to create : " << tarEdgeCnt << endl;

    while ( tarEdgeCnt > curEdgeCnt ) {

       
        
        
        int rIdx = rand() % edgeSampling.size();
        int edgeLength = edgeSampling[rIdx]; //edgeDist_.sample();

        Gain maxG(-INT_MAX, nullptr, nullptr);
       
        std::random_shuffle(srcBins.begin(), srcBins.end());
        // Iterate source-sink bins (distance(source, sink) == edgeLength)
        for(Bin* srcBin : srcBins) {
            vector<Bin*> sinkBins = getSinkBins(srcBin, edgeLength, true);
            // Iterate sink bins
            for(Bin* sinkBin : sinkBins) {
                Gain localMaxG = getMaxGain(srcBin, sinkBin);

                if(maxG.value() < localMaxG.value()) {
                    
                    maxG = localMaxG;
            
                }
            }
        }

        if( maxG.value() == -INT_MAX ) {

        } else {

            Node* srcNode = maxG.n1();
            Node* sinkNode = maxG.n2();
            connect(srcNode, sinkNode);
            curEdgeCnt++;

            if(curEdgeCnt % 1000 == 0 || curEdgeCnt == tarEdgeCnt) { 
                double progress = 1.0 * curEdgeCnt / tarEdgeCnt;
                printf("distribution matching progress... [%2.2f\%]\n", 100* progress); 
            }

        }
        //
    }

    print();


    // 
}

// 현재 Source Bin과 Distance가 edgeLength인 sinkBin 리스트를 반환
vector<Bin*> Netlist::getSinkBins(Bin* srcBin, int edgeLength, bool shuffle) {

    //cout << "getSinkBins" << endl;

    vector<Bin*> sinkBins;

    for(int dx=0; dx <= edgeLength; dx++) {
        int dy = edgeLength - dx;
        int x = srcBin->x() + dx;
        int y = srcBin->y() + dy;

        if( x >= layoutDimX() || y >= layoutDimY() ) 
            continue;
        
        Bin* sinkBin = getBin(x, y);
        sinkBins.push_back(sinkBin);
    }

    if( shuffle && sinkBins.size() > 1 )
        std::random_shuffle(sinkBins.begin(), sinkBins.end());

    return sinkBins;
}

// Source Bin과 Sink Bin에 속한 Node중 가장 큰 Gain을 가지는 Node쌍을 반환
Gain Netlist::getMaxGain(Bin* srcBin, Bin* sinkBin) {

    //cout << "getMaxGain" << endl;

    // for debug
    double _foG, _fiG, _bboxG;
    int _fo, _fi, _bbox1, _bbox2;
    //

    double maxG = -INT_MAX;
    int maxGain = INT_MIN;
    Node *srcNode, *sinkNode;

    for(int fo=0; fo<foDist_.xMax(); fo++) {
        vector<Node*> candiSrcs = srcBin->fo2Nodes(fo);

        if(candiSrcs.size() == 0) continue;
        double foG = fanoutGain(fo, fo+1);

        double bboxG = -INT_MAX;
        // srcBin에 속한 node중 sinkBin과 연결됬을때
        // netBbox gain이 가장큰 src node를 반환
        
        std::random_shuffle(candiSrcs.begin(), candiSrcs.end());

        Node* localMaxSrcNode;
        //for(int i=0; i < candiSrcs.size(); i++) {
        //    Node* candiSrc = candiSrcs[i];
        for(Node* candiSrc : candiSrcs) {

            if(candiSrc->getType() == NodeType::PrimaryOut)
                continue;
            
            int lx, ly, ux, uy;
            int x, y;
            lx = candiSrc->lx(), ly = candiSrc->ly();
            ux = candiSrc->ux(), uy = candiSrc->uy();
            x = sinkBin->x(), y = sinkBin->y();
          
            int bbox1 = (ux - lx) + (uy - ly);
            int bbox2 = (max(ux, x) - min(lx, x)) + (max(uy, y) - min(ly, y));
            double localBboxG = 0;

            if( bbox1 < bbox2 ) {
                //lx > x || ly > y || ux < x || uy < y ) {
                localBboxG = bboxGain( bbox1, bbox2 );
            }

            //if( localBboxG < 0 ) continue;

            if( bboxG < localBboxG ) {
                bboxG = localBboxG;
                localMaxSrcNode = candiSrc;
            } else {
            }

        }

        if( bboxG == -INT_MAX ) continue;

        // 여기까지 srcBin에 속한 Node중 fanout, bbox Gain의 기댓값이 가장큰
        // srcNode를 찾음.
        // 이제부터 sinkBin에 속한 Node중 fanin Gain값이 가장큰 sinkNode를 찾아야됨.
        for(int fi=0; fi<fiDist_.xMax(); fi++) {
            vector<Node*> candiSinks = sinkBin->fi2Nodes(fi);

            



            if(candiSinks.size() == 0) continue;


            double fiG = faninGain(fi, fi+1);

            double totG = 1.0 * fiG + 1.0 * foG + 1.0 * bboxG;

            if( totG > maxG ) {
                std::random_shuffle(candiSinks.begin(), candiSinks.end());
                for(Node* candiSink : candiSinks) {
                   
                    // exception case handling
                    if(candiSink->getType() == NodeType::PrimaryIn) {
                        continue;
                    } else if(candiSink->getType() == NodeType::PrimaryOut) {
                        if( candiSink->numFanins() >= 1 ) continue;
                    } else { 
                        // if srcBin == sinkBin
                        if( srcBin == sinkBin && candiSink <= localMaxSrcNode )
                            continue;

                        if( localMaxSrcNode->hasConnection(candiSink) )
                            continue;
                    } 
                    
                    //if(candiSink != localMaxSrcNode) {
                        srcNode = localMaxSrcNode;
                        sinkNode = candiSink;
                        maxG = totG;
                    //}
                }
            }
        }
    }

    if(maxG == -INT_MAX) {
    } else {
    }
    
    return Gain(maxG, srcNode, sinkNode);
}

// Bbox gain function
double Netlist::bboxGain(int bbox1, int bbox2) {

    double currErr = 1.0 * abs(bboxDist_.delta(bbox1)) + 1.0 * abs(bboxDist_.delta(bbox2));
    double nextErr = 1.0 * abs(bboxDist_.delta(bbox1)-1) + 1.0 * abs(bboxDist_.delta(bbox2)+1);

    //    - 1.0 * abs(bboxDist_.delta(bbox1)-1);
    //double nextErr = 1.0 * abs(bboxDist_.delta(bbox2)) - 1.0 * abs(bboxDist_.delta(bbox2)+1);
    
    double deltaErr = currErr - nextErr;
    //double reward = (fi1 == 0) ? 16 : 0; 
    //double g = deltaErr + reward;
    //return g;
    return deltaErr;
    


    
    double deltaR1 = bboxDist_.deltaErrorRatio(bbox1, -1);
    double deltaR2 = bboxDist_.deltaErrorRatio(bbox2, 1);
    double errorR1 = bboxDist_.errorRatio(bbox1);
    double errorR2 = bboxDist_.errorRatio(bbox2);
    double gain = deltaR1 + deltaR2 + errorR1 - errorR2;
    //cout << "bbox gain : " << gain << endl;
    return gain;
}

// Fanout gain function
double Netlist::fanoutGain(int fo1, int fo2) {
    double currErr = 1.0 * abs(foDist_.delta(fo1)) + 1.0 * abs(foDist_.delta(fo2));
    double nextErr = 1.0 * abs(foDist_.delta(fo1)-1) + 1.0 * abs(foDist_.delta(fo2)+1);
    double deltaErr = currErr - nextErr;
    double reward = (fo1 == 0) ? 16 : 0; 
    double g = deltaErr + reward;
    return g;
 
    //double deltaR1 = foDist_.deltaErrorRatio(fo1, -1);
    //double deltaR2 = foDist_.deltaErrorRatio(fo2, 1);
    //double gain = deltaR1 + deltaR2;
    //double errorR1 = foDist_.errorRatio(fo1);
    //double errorR2 = foDist_.errorRatio(fo2);
    //double gain = deltaR1 + deltaR2 + errorR1- errorR2;
    //cout << "fanout gain : " << gain << endl;
    //return gain;
}

// Fanin gain function
double Netlist::faninGain(int fi1, int fi2) {
    double currErr = 1.0 * abs(fiDist_.delta(fi1)) + 1.0 * abs(fiDist_.delta(fi2));
    double nextErr = 1.0 * abs(fiDist_.delta(fi1)-1) + 1.0 * abs(fiDist_.delta(fi2)+1);
    double deltaErr = currErr - nextErr;
    double reward = (fi1 == 0) ? 16 : 0; 
    double g = deltaErr + reward;
    return g;
    //double deltaR1 = fiDist_.deltaErrorRatio(fi1, -1);
    //double deltaR2 = fiDist_.deltaErrorRatio(fi2, 1);
    //double gain = deltaR1 + deltaR2;
    //double errorR1 = fiDist_.errorRatio(fi1);
    //double errorR2 = fiDist_.errorRatio(fi2);
    //double gain = deltaR1 + deltaR2 + errorR1- errorR2;
    //cout << "fanin gain : " << gain << endl;
    //return gain;
}


}
