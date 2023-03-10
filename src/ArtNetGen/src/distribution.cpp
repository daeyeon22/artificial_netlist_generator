//#include "distribution.h" 
#include "artnetgen/artNetGen.h"
//#include <math.h>
#include <algorithm>
#include <cmath>

namespace artnetgen {

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::ceil;
using std::max;
using std::min;
using std::abs;

DistributionInfo::DistributionInfo(
        int value, int target, int count, double ratio) :
    value_(value),
    target_(target),
    count_(count),
    ratio_(ratio) {}

DistributionInfo::~DistributionInfo() {
    value_ = target_ = count_ = ratio_ = 0;
}

void
DistributionInfo::print() {
    std::cout << "  " << value_
        << " " << target_
        << " " << count_
        << " " << ratio_
        << std::endl;
}

Distribution::Distribution() {
}

Distribution::Distribution(Distribution* d) {
    min_ = d->xMin();
    max_ = d->xMax();

    cout << min_ << " " << max_ << endl;

    totalCnt_ = d->totalCnt();
    description_ = d->description();

    for(auto it = d->begin(); it != d->end(); it++) {
        int value = it->value();
        int count = it->count();
        int target = it->target();
        double ratio = it->ratio();
        info_.push_back(DistributionInfo(value, target, count, ratio));
    }
}

Distribution::~Distribution() {

}

void 
Distribution::setDescription(string description) {
    description_ = description;
}


void Distribution::print(int maxRows) {
    int total=totalCnt_;

    cout << "[INFO] " << description_ << endl;
    for(int x=min_; x<= max_; x++) {
        auto it = info_[x];
        int target = it.target();
        int count = it.count();
        int value = it.value();
        double tarRatio = 1.0 * target/total;
        double curRatio = 1.0 * count/total;

        int numMarks = 20;
        double div = 100/numMarks;
        int tarMarks = int((tarRatio*100)/div);
        int curMarks = int((curRatio*100)/div);
        printf("[%2d] ", value);
        for(int i=0; i<20; i++) {
            if(i<tarMarks) {
                if(i<curMarks) {
                    cout << "*";
                } else {
                    cout << "-";
                }
            } else {
                if(i<curMarks) {
                    cout << "+";
                } else {
                    cout << " ";
                }
            }
        }
        printf(" (%.3f/%.3f)",tarRatio, curRatio);
        cout << endl;
        if(x > min_+maxRows) {
            cout << "..." << endl;
            break;
        }
    }



}


void
Distribution::print() {
    //cout << endl;
    cout << " - " <<  description_ <<  std::endl;
    int numRows = min(max_, min_+15);
    for(int x=min_; x <= numRows; x++) {
        for(int cX=x; cX <= max_; cX+=15) {
            auto it = info_[cX];
            printf("(%2d) %6d/%6d    ", it.value(), it.target(), it.count());
        }
        printf("\n");
    }

    //printf("-----------------------    -----------------------\n");
    for(auto& it : info_) {
        //printf("[%2d] %6d %6d\n", it.value(), it.target(), it.count());
        //it.print();
    }
}

double Distribution::targetAvg() const {
    double avg = 0.0;
    for(auto& it : info_) 
        avg += it.value() * it.target();
    avg /= totalCnt_;
    return avg;
}

double Distribution::currentAvg() const {
    double avg = 0.0;
    for(auto& it : info_) 
        avg += it.value() * it.count();
    avg /= totalCnt_;
    return avg;
}

void 
Distribution::incr(int x, int cnt) {
    info_[x].incr(cnt);
}

void
Distribution::decr(int x, int cnt) {
    info_[x].decr(cnt);
}

void
Distribution::init(int min, int max, int totalCnt, std::vector<DistributionInfo> info) {    
    min_ = min;
    max_ = max;
    totalCnt_ = totalCnt;

    int totalTarCnt = 0;
    int totalCurCnt = 0;
    double totalTarRatio = 0.0;
    vector<double> tr; //(0.0, max+1);

    for(auto& it : info) {

        int value = it.value();
        int target = it.target();
        int count = it.count();
        double ratio = it.ratio();
        totalTarRatio += ratio;
        
        while(tr.size() < value) 
            tr.push_back(0.0);
        
        tr.push_back(ratio);
        
    }

    for(int value=0; value <= max; value++) {
        tr[value] /= totalTarRatio;
        double ratio = tr[value];
        int target = ceil(1.0* totalCnt * ratio);
        int count = 0;
        
        if(totalTarCnt + target > totalCnt)
            target = totalCnt - totalTarCnt;

        totalCurCnt += count;
        totalTarCnt += target;

        info_.push_back(DistributionInfo(value, target, count, ratio));
    }

    if(totalCnt - totalCurCnt > 0) 
        info_.front().setCount(totalCnt -totalCurCnt);


    double tmp = 0.0;

    for(auto r:tr) tmp+=r;
    //cout << "total: " << tmp << endl;

    /*
    //info_ = std::vector<DistributionInfo>(max_);

    int totalCurCnt = 0;
    int totalTarCnt = 0;

    for(auto& it : info) {
        int value = it.value();
        double ratio = it.ratio();
        int target = ceil(1.0 * totalCnt * ratio);
        int count = 0; //ceil(1.0 * totalCnt * ratio);

        if(totalTarCnt + target > totalCnt)
            target = totalCnt - totalTarCnt;

        while( info_.size() < value ) {
            info_.push_back(DistributionInfo(info_.size(), 0, 0, 0.0));
        }

        
        totalCurCnt += count;
        totalTarCnt += target;

        info_.push_back(DistributionInfo(value, target, count, ratio)); 
    }


    if(totalTarCnt != totalCnt) {
        cout << "total count is modified" << endl;
        cout << totalCnt << " -> " << totalTarCnt << endl;

        totalCnt_ = totalTarCnt;
    }
    



    int totalDiff = totalCnt_ - totalCurCnt;

    if( totalDiff > 0 ) {
        info_[0].setCount(totalDiff);
    }
    */
    //print();
}

void 
Distribution::clear() {
    description_ = "";
    min_ = max_ = 0;
    totalCnt_ = 0;
    info_.clear();
}

vector<int> Distribution::getSamplingVector() {
    vector<int> sampling;

    for(int x=0; x < max_+1; x++) {
        int cnt = target(x);
        sampling.insert(sampling.end(), cnt, x);
    }

    std::random_shuffle(sampling.begin(), sampling.end());
    return sampling;
}


int Distribution::sample() {
    vector<int> lb(max_+1);
    vector<int> ub(max_+1);
    int maxRand = 0;
    for(int x=0; x < max_+1; x++) {
        int dY = delta(x);
        dY = (dY < 0) ? std::abs(dY) : 0;
        
        if( x==0 )
            lb[x] = 0;
        else
            lb[x] = ub[x-1];

        ub[x] = lb[x] + dY;
        maxRand = std::max(maxRand, ub[x]);
    }

    int rv = std::rand() % maxRand;

    int sampled = std::lower_bound(ub.begin(), ub.end(), rv) - ub.begin();

    //cout << "sampled : " << sampled << endl;
    return sampled;
}

double Distribution::errorRatio(int x) const {
    double tarRatio = 1.0 * target(x) / totalCnt_;
    double curRatio = 1.0 * count(x) / totalCnt_;

    return curRatio - tarRatio;

}

double Distribution::deltaErrorRatio(int x, int dy) const {

    int prevCnt = count(x);
    int nextCnt = count(x) + dy;
    int targetCnt = target(x);
    double prevRatio = 1.0 * prevCnt / totalCnt_;
    double nextRatio = 1.0 * nextCnt / totalCnt_;
    double targetRatio = 1.0 * targetCnt / totalCnt_;

    double errorR1 = abs(prevRatio - targetRatio);
    double errorR2 = abs(nextRatio - targetRatio);

    double deltaR = errorR1 - errorR2;
    return deltaR;
}

double Distribution::currentRatio(int x) const {
    int currentCnt = count(x);
    double ratio = 1.0 * currentCnt / totalCnt_;
    return ratio;
}

double Distribution::targetRatio(int x) const {
    int targetCnt = target(x);
    double ratio = 1.0 * targetCnt / totalCnt_;
    return ratio;
}



};
