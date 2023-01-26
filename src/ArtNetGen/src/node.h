#ifndef __ARTNETGEN_NODE_HEADER__
#define __ARTNETGEN_NODE_HEADER__

#include <vector>
#include <string>
#include <unordered_map>
//#include "opendb/db.h"


namespace odb {
class dbMaster;
class dbMTerm;
};

namespace artnetgen {

class Bin;
class Net;
class Node;

enum NodeType {
    PrimaryIn, PrimaryOut, ClockIn, ResetIn, Combinational, Sequential
};

enum TermType {
    Primary, Reset, Clock, Signal
};

enum DirType {
    Input, Output, InOut
};

class Net {
  private:
    std::string name_;
    std::vector<std::pair<Node*, std::string>> terms_;

  public:
    Net();
    //Net(std::string name);
    //Net(const Net& net);
    ~Net();
    void setName(std::string name);
    void addTerm(Node* node, std::string termName);
    void print();

    std::string getName();

    std::vector<std::pair<Node*, std::string>> getTerms();
};


class Node {
  private:
    std::string name_;
    //std::string masterName_;
    
    // used for technology mapping
    odb::dbMaster* master_;
    std::vector<std::string> inSigTerms_;
    std::vector<std::string> outSigTerms_;
    std::vector<std::string> inClkTerms_;
    std::vector<std::string> inRstTerms_;
    std::unordered_map<Node*, std::string> node2term_;

    NodeType type_;
    // used for distribution matching
    // Source-Sinks 
    // Net bounding box (bbox)
    int lx_, ly_, ux_, uy_;
    Bin* bin_;
    std::vector<Node*> sources_;
    std::vector<Node*> sinks_;
    
   
    //bool hasIo_;


  public:
    Node();
    ~Node();

    int numFanins() const;
    int numFanouts() const;
    int x() const;
    int y() const;
    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;
    int bboxSize() const;


    bool isPIO() const;

    std::vector<Node*> getSinks();
    std::vector<Node*> getSources();


    odb::dbMaster* getDbMaster();
    //odb::dbMTerm* getDbMTerm(Node* sink);

    std::string getTerm(Node* node);
    std::string getName();
    NodeType getType();
    Bin* getBin();
    void setName(std::string name);
    void removeAllSinks();
    void removeAllSources();

    void setType(NodeType type);
    void setBin(Bin* bin);
    void setBbox(int lx, int ly, int ux, int uy);

    int numOutTerms();
    //void setMasterName(std::string masterName_);
    void setDbMaster(odb::dbMaster* master);
    void mappingTerms(Node* clkIn, Node* rstIn); //mapDbMTerms();
    
    void connectClock(Node* clkIn);
    void connectReset(Node* rstIn);

    void removeSource(Node* target);
    void removeSink(Node* target);

    void addSource(Node* src);
    void addSink(Node* sink);
    void addSinks(std::vector<Node*> sinks);
    void addSources(std::vector<Node*> sources);
   
    bool hasConnection(Node* target);

    bool hasMultiPortConn();

};

};

#endif
