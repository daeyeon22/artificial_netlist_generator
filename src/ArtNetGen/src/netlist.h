#include <iostream>
#include <vector>
#include <unordered_map>


namespace odb {
    class dbMaster;
};

namespace artnetgen {

class Node;
class Bin;
class Net;


class Gain {
  private:
    double value_;
    Node* n1_;
    Node* n2_;
  public:
    Gain(double value, Node* n1, Node* n2):
        value_(value), n1_(n1), n2_(n2) {}
    ~Gain(){}
    Gain(const Gain& g) : value_(g.value_), n1_(g.n1_), n2_(g.n2_) {}
    //    value_ = g.value();
    //    n1_ = g.n1();
    //    n2_ = g.n2();
    //}

    double value() const { return value_; }
    Node* n1() { return n1_; }
    Node* n2() { return n2_; }
};

class Netlist {
  private:
    int layoutDimX_;
    int layoutDimY_;
    
    ArtNetGen* ang_;

    //std::vector<Node> addStor_;
    //std::vector<Node> nodeStor_;
    std::vector<Bin> binStor_;
    std::vector<Net> netStor_;

    std::vector<Node*> nodes_;
    std::vector<Bin*> bins_;
    std::vector<Net*> nets_;


    std::vector<Node*> primIns_;
    std::vector<Node*> primOuts_;

    Distribution fiDist_;
    Distribution foDist_;
    Distribution bboxDist_; 
    Distribution edgeDist_;

    std::vector<std::vector<odb::dbMaster*>> fi2SequMasters_;
    std::vector<std::vector<odb::dbMaster*>> fi2CombMasters_;
    std::unordered_map<odb::dbMaster*, double> master2ratio_;
    //private functions
    //std::vector<Node*>::iterator nodes();
    //std::vector<Bin*>::iterator bins();

    Node* createMergeNode(Node* n1, Node* n2, bool front);
    Node* createMergeNode(Node* n1, Node* n2, Node* n3, bool front);
    void connect(Node* source, Node* sink);
    void distMatching();
    void createPrimaryIO();
    void setPrimaryIO();
    void summaryDesign();

    //void checkUnconnected();
    void initialize(); //ArtNetGen* ang);

    //
    void timingPathConstruction_v1();
    void timingPathConstruction_v2();
    void insertSequentialNode(Node* target, bool front);
    void technologyMapping();

    int getCountNodeType(int type);

    Bin* getBin(int x, int y);
    Gain getMaxGain(Bin* srcBin, Bin* sinkBin);
    std::vector<Bin*> getSinkBins(Bin* srcBin, int edgeLength, bool shuffle);
    std::unordered_map<Node*, int> topologicalSort();
    int getMaxTopologicalOrder();
    double getAvgTopologicalOrder();
    double getCombinationalRatio();
    double bboxGain(int bbox1, int bbox2);
    double fanoutGain(int fo1, int fo2);
    double faninGain(int fi1, int fi2);


    bool hasMaster(int numFanin, bool isSequential);
    void initOnlyUseMasters();


  public:
    Netlist();
    ~Netlist();


    int layoutDimX() { return layoutDimX_; }
    int layoutDimY() { return layoutDimY_; }
    void generate();
    void setArtNetGen(ArtNetGen* ang);
    void print();
    void readDb(odb::dbDatabase* db);
    
    std::vector<Net*> getNetlist();
    std::vector<Node*> getNodes();
    std::vector<Node*> getPrimaryInputs();
    std::vector<Node*> getPrimaryOutputs();

};


};

