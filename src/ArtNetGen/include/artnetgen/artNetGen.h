#ifndef __ARTNETGEN_HEADER__
#define __ARTNETGEN_HEADER__

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace odb {
class dbDatabase;
class dbInst;
class dbMaster;
}

namespace sta {
class dbSta;
}

namespace artnetgen {




class MasterInfo {
  private:
    odb::dbMaster* master_;
    double ratio_;

  public:
    MasterInfo(odb::dbMaster* master, double ratio);
    //MasterInfo(odb::dbMaster* master, double ratio, int inCnt, int outCnt, bool hasClock);
    ~MasterInfo();
    odb::dbMaster* master() const { return master_; }
    double ratio() const { return ratio_; }
    void print();
};

class DistributionInfo {
  private:
    int value_;
    int count_;
    int target_;
    double ratio_;
  public:
    DistributionInfo( int value, int target, int count, double ratio );
    ~DistributionInfo();
    int value() const { return value_; }
    int count() const { return count_; }
    int target() const { return target_; }
    double ratio() const { return ratio_; } 
    int error() const { return std::abs(count_ - target_); }
    int delta() const { return count_ - target_; }
    void incr(int cnt) { count_ += cnt; }
    void decr(int cnt) { count_ -= cnt; }
    void setCount(int count) { count_ = count; }
    void setValue(int value) { value_ = value; }
    void setTarget(int target) { target_ = target; }
    void print();
};  


class Distribution {
  private:
    std::string description_;
    int min_, max_;
    int totalCnt_;
    std::vector<DistributionInfo> info_;

  public:

    Distribution();
    Distribution(Distribution* d);
    ~Distribution();
    void init(int min, int max, int totalCnt, std::vector<DistributionInfo> info);
    int xMin() const { return min_; }
    int xMax() const { return max_; }
    double targetAvg() const;
    double currentAvg() const;
    int target(int x) const { return info_[x].target(); }
    int count(int x) const { return info_[x].count(); }
    int sample();
    void setDescription(std::string description);
    void incr(int x, int cnt=1);
    void decr(int x, int cnt=1);
    int delta(int x) { return info_[x].delta(); }
    int error(int x) { return info_[x].error(); }
    int totalCnt() const { return totalCnt_; }
    double currentRatio(int x) const;
    double targetRatio(int x) const;
    double deltaErrorRatio(int x, int dy) const;
    double errorRatio(int x) const;
    void clear();
    void print();
    void print(int maxRows);
    std::string description() { return description_; }
    std::vector<int> getSamplingVector(); 
    std::vector<DistributionInfo>::iterator end() { return info_.end(); }
    std::vector<DistributionInfo>::iterator begin() { return info_.begin(); }
};  

/*
class DistributionInfo {
  private:
    int value_;
    int count_;
    double ratio_;
  public:
    DistributionInfo( int value, int count, double ratio );
    ~DistributionInfo();
    int value() const { return value_; }
    int count() const { return count_; }
    double ratio() const { return ratio_; } 
    void print();
};  
*/

class Logger;
class Netlist;

class ArtNetGen {
  public:
    void setDb(odb::dbDatabase* db);
    void setSta(sta::dbSta* sta);
    void init();
    void clear();

    // run
    void run(); //generate(); 
    
    // setter for param
    void setInstanceCnt(int instanceCnt) { instanceCnt_ = instanceCnt; }
    void setInputPinCnt(int inputPinCnt) { inputPinCnt_ = inputPinCnt; }
    void setOutputPinCnt(int outputPinCnt) { outputPinCnt_ = outputPinCnt; }
    //void setSynClkPeriod(double synClkPeriod) { synClkPeriod_ = synClkPeriod; }
    void setAvgGateDelay(double avgGateDelay); // { avgGateDelay_ = avgGateDelay; }
    void setAvgTopoOrder(double avgTopoOrder) { avgTopoOrder_ = avgTopoOrder; }

    
    void setCombRatio(double combRatio) { combRatio_ = combRatio; }
    void setVerbose(int verbose) { verbose_ = verbose; } 
    void setSpecFile(const char* specFile) { specFile_ = specFile; }
    void setOutFile(const char* outFile) { outFile_ = outFile; }
    void setTopModule(const char* topModule) { topModule_ = topModule; }
    void setDontUse(const char* macroName);


    int getInstanceCnt() const { return instanceCnt_; }
    int getInputPinCnt() const { return inputPinCnt_; }
    int getOutputPinCnt() const { return outputPinCnt_; }
    //double getSynClkPeriod() const { return synClkPeriod_; }
    double getAvgGateDelay() const { return avgGateDelay_; }
    double getAvgTopoOrder() const { return avgTopoOrder_; }
    double getCombRatio() const { return combRatio_; }
    int getMaxFanin() const { return maxFanin_; }
    int getMaxFanout() const { return maxFanout_; }

   
    void printMasters();
    void writeVerilog(const char* fileName);
    void writeSdc(const char* fileName);
    // synClkPeriod, avgGateDelay will be disappeared later
    void writeSpec(const char* fileName);
    // create spec file from parameters
    void createSpec(int numInsts, int numPIOs, float combRatio, float avgNegDeg, float avgBbox, float avgTopoOrder, 
            const char* onlyUseList, const char* fileName);

    double analyzeTopologicalOrder();


    //void analyzeTopoOrder();


    Distribution* fiDist() { return &fiDist_; }
    Distribution* foDist() { return &foDist_; }
    Distribution* edgeDist() { return &edgeDist_; }
    Distribution* bboxDist() { return &bboxDist_; }

    std::vector<MasterInfo> getMasterInfo() { return masters_; }
    std::vector<DistributionInfo> getFaninDistInfo() { return fanIns_; }
    std::vector<DistributionInfo> getFanoutDistInfo() { return fanOuts_; }
    std::vector<DistributionInfo> getBboxDistInfo() { return netBboxes_; }
    std::vector<DistributionInfo> getEdgeDistInfo() { return edges_; }

    ArtNetGen ();
    ~ArtNetGen ();

    

  private:
    odb::dbDatabase* db_;
    sta::dbSta* sta_;
    std::shared_ptr<artnetgen::Logger> log_;

    Netlist* artnet_;

    // param
    int instanceCnt_;
    int inputPinCnt_;
    int outputPinCnt_;
    
    double avgGateDelay_;
    //double synClkPeriod_;
    double avgTopoOrder_;
    double combRatio_;

    // dont use
    //int binSqrt_;
    //int maxChainDepth_;
    int verbose_;
    std::string specFile_;
    std::string outFile_;
    std::string topModule_;

    // privateFuncs
    void readSpec();
    void writeVerilog(); 

    //void readSpec(const char* fileName);
    //void writeVerilog(const char* fileName);
    void printAllVars();
    void checkMasters();

    Distribution fiDist_;
    Distribution foDist_;
    Distribution edgeDist_;
    Distribution bboxDist_;

    int maxFanin_;
    int maxFanout_;

    std::vector<MasterInfo> masters_;
    std::vector<DistributionInfo> fanIns_;
    std::vector<DistributionInfo> fanOuts_;
    std::vector<DistributionInfo> netBboxes_;
    std::vector<DistributionInfo> edges_;
    
};

// helper function
std::unordered_map<std::string, int> getPortInfo(odb::dbMaster* master);

}

#endif
