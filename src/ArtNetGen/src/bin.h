#ifndef __ARTNETGEN_BIN_HEADER__
#define __ARTNETGEN_BIN_HEADER__

#include <vector>
#include <iostream>
#include <unordered_map>
#include <map>

namespace artnetgen {
class Node;
class Bin
{
  private:
    int x_, y_;
    std::vector<Node*> nodes_;
    std::vector<std::vector<Node*>> fi2Nodes_;
    std::vector<std::vector<Node*>> fo2Nodes_;
    std::unordered_map<Node*,int> fi_;
    std::unordered_map<Node*,int> fo_;
    //std::map<Node*,int> fi_;
    //std::map<Node*,int> fo_;

  public:

    Bin();
    ~Bin();

    void init(int maxFi, int maxFo);
    void setCoord(int x, int y);
    void addNode(Node* node);
    void update(Node* node);
    int x() const;
    int y() const;
    int numNodes() const;
    std::vector<Node*> fi2Nodes(int fi);
    std::vector<Node*> fo2Nodes(int fo);
    Node* getRandomNode();
};

};

#endif
