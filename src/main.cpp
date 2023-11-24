#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "GraphTheory.h"

class cItem
{
    static std::vector<cItem *> theItems;
    std::string myName;

public:
    cItem(const std::string &name)
        : myName(name)
    {
    }
    std::string name() const
    {
        return myName;
    }
    static cItem *findOrCreate(const std::string &name)
    {
        auto it = std::find_if(
            theItems.begin(), theItems.end(),
            [&name](cItem *pi)
            {
                return pi->myName == name;
            });
        if (it == theItems.end())
        {
            theItems.push_back(
                new cItem(name));
            return theItems.back();
        }
        return *it;
    }
    static int count()
    {
        return theItems.size();
    }
};

class cHold
{
    static std::vector<cHold *> theHolds;
    cItem *myItem;
    int myCount;

public:
    cHold(
        const std::string &item,
        int count)
        : myItem(cItem::findOrCreate(item)),
          myCount(count)
    {
    }
    std::string name() const
    {
        return myItem->name();
    }
    int count() const
    {
        return myCount;
    }
    static std::vector<cHold *> get()
    {
        return theHolds;
    }
    static void add(
        const std::string &item,
        int count)
    {
        theHolds.push_back(
            new cHold(item, count));
    }
};

class cTrade
{
    static std::vector<cTrade *> myTrades;
    cItem *myGet;
    cItem *myGive;

public:
    cTrade(
        const std::string &getName,
        const std::string &giveName)
        : myGet(cItem::findOrCreate(getName)),
          myGive(cItem::findOrCreate(giveName))
    {
    }
    std::string getName() const
    {
        return myGet->name();
    }
    std::string giveName() const
    {
        return myGive->name();
    }
    static std::vector<cTrade *>
    get()
    {
        return myTrades;
    }
    static void add(
        const std::string &getName,
        const std::string &giveName)
    {
        myTrades.push_back(
            new cTrade(getName, giveName));
    }
};

class cFlowGraph
{
    raven::graph::sGraphData gd;
    std::vector<int> vLinkFlow;

    bool isTrade(std::pair<int, int> &link)
    {
        return (
            gd.g.userName(link.first).find("_") == -1 &&
            gd.g.userName(link.second).find("_") == -1);
    }

    int flow(std::pair<int, int> &link)
    {
        return vLinkFlow[gd.g.find(link.first, link.second)];
    }

public:
    void make();
    void displayLinks();
    void calculate();
    bool isFeasible();
    void displayTrades();
};

std::vector<cItem *> cItem::theItems;
std::vector<cHold *> cHold::theHolds;
std::vector<cTrade *> cTrade::myTrades;

void generate1()
{
    cHold::add("1", 3);
    cHold::add("2", 0);
    cHold::add("3", 0);
    cHold::add("4", 1);

    cTrade::add("2", "1");
    cTrade::add("3", "2");
}

void cFlowGraph::make()
{
    gd.g.clear();
    gd.g.directed();
    int maxLinkCount = cItem::count() * cItem::count();
    const int infinite = 2e9;
    gd.edgeWeight.resize(maxLinkCount, infinite);

    for (cHold *hold : cHold::get())
    {
        if (hold->count() > 1)
        {
            // source
            std::string name = "source_" + hold->name();
            gd.g.add(
                "all_source",
                name);
            int ie = gd.g.add(
                name,
                hold->name());
            gd.edgeWeight[ie] = hold->count() - 1;
        }
        else if (!hold->count())
        {
            // sink
            int ie = gd.g.add(
                hold->name(),
                "sink_" + hold->name());
            gd.edgeWeight[ie] = 1;
            gd.g.add(
                "sink_" + hold->name(),
                "all_sink");
        }
    }

    for (cTrade *trade : cTrade::get())
    {
        gd.g.add(
            trade->giveName(),
            trade->getName());
    }
}
void cFlowGraph::displayLinks()
{
    for (auto &link : gd.g.edgeList())
    {
        int ei = gd.g.find(link.first, link.second);
        std::cout
            << " flow " << vLinkFlow[ei]
            << " in " << std::setw(12) << gd.g.userName(link.first)
            << " -> " << gd.g.userName(link.second)
            << "\n";
    }
}

void cFlowGraph::calculate()
{
    vLinkFlow.clear();
    vLinkFlow.resize(gd.g.edgeCount(), 0);
    gd.startName = "all_source";
    gd.endName = "all_sink";

    flows(
        gd,
        vLinkFlow);

    displayLinks();
}

bool cFlowGraph::isFeasible()
{
    // check that every flow into an item sink is 1
    // meaning that the missing item was found
    for (auto &link : gd.g.edgeList())
    {
        if (gd.g.userName(link.second).find("sink_") == 0)
            if (flow( link ) != 1)
                return false;
    }
    return true;
}

void cFlowGraph::displayTrades()
{
    std::cout << "\n";
    for (auto &link : gd.g.edgeList())
    {
        if (isTrade(link))
            std::cout << "trade " << gd.g.userName(link.first)
                      << " - > " << gd.g.userName(link.second)
                      << " times " << flow( link )
                      << "\n";
    }
}

main()
{
    // generate test problem
    generate1();

    // create the flow graph
    cFlowGraph fg;
    fg.make();

    // calculate the flows
    fg.calculate();

    // check that all missing items can be traded for
    if (!fg.isFeasible())
    {
        std::cout << "problem is not feasible\n";
        return 1;
    }

    // display reuired trades
    fg.displayTrades();

    return 0;
}
