#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "GraphTheory.h" // Pathfinder library https://github.com/JamesBremner/PathFinder

/// @brief //////////////////////////////////////
// An item that can be traded

class cItem
{
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

    /// @brief Find or create an item
    /// @param name name of item to find
    /// @return pointer to found item

    static cItem *findOrCreate(const std::string &name);

    static int count();
};

////////////////////////////////////////////////
/// Items held by trader

class cHold
{
    cItem *myItem; // the item held
    int myCount;   // the number of this item held

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
    static std::vector<cHold *> get();

    static void add(
        const std::string &item,
        int count);
};

////////////////////////////////////////////////////////////////////

/// @brief  The possible trades
class cTrade
{

    cItem *myGet;  // item required in trade
    cItem *myGive; // the item ofered in trade

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
    get();

    /// @brief Add a possible trade
    /// @param getName  // the name of the item required
    /// @param giveName // the name of the item offered

    static void add(
        const std::string &getName,
        const std::string &giveName);
};

/// @brief The graph representing the problem
class cFlowGraph
{
    raven::graph::sGraphData gd;
    std::vector<int> vLinkFlow;

    /// @brief true if link represents a trade
    /// @param link
    /// @return
    bool isTrade(std::pair<int, int> &link)
    {
        return (
            gd.g.userName(link.first).find("_") == -1 &&
            gd.g.userName(link.second).find("_") == -1);
    }

    /// @brief The flow through a link
    /// @param link
    /// @return
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

struct sDataStore
{
    std::vector<cItem *> theItems;
    std::vector<cHold *> theHolds;
    std::vector<cTrade *> myTrades;
};

sDataStore theDataStore;

cItem *cItem::findOrCreate(const std::string &name)
{
    // check if item already exists
    auto it = std::find_if(
        theDataStore.theItems.begin(), theDataStore.theItems.end(),
        [&name](cItem *pi)
        {
            return pi->myName == name;
        });

    if (it == theDataStore.theItems.end())
    {
        // create and add a new item
        theDataStore.theItems.push_back(
            new cItem(name));
        return theDataStore.theItems.back();
    }
    // return pointer to item that previosly existed
    return *it;
}
int cItem::count()
{
    return theDataStore.theItems.size();
}

std::vector<cHold *> cHold::get()
{
    return theDataStore.theHolds;
}
void cHold::add(
    const std::string &item,
    int count)
{
    theDataStore.theHolds.push_back(
        new cHold(item, count));
}
std::vector<cTrade *>
cTrade::get()
{
    return theDataStore.myTrades;
}

/// @brief Add a possible trade
/// @param getName  // the name of the item required
/// @param giveName // the name of the item offered

void cTrade::add(
    const std::string &getName,
    const std::string &giveName)
{
    theDataStore.myTrades.push_back(
        new cTrade(getName, giveName));
}
void generate1()
{
    // initial holding of each item
    cHold::add("1", 3);
    cHold::add("2", 0);
    cHold::add("3", 0);
    cHold::add("4", 1);

    // possible trades
    cTrade::add("2", "1");
    cTrade::add("3", "2");
}

void cFlowGraph::make()
{
    // setup flow graph
    gd.g.clear();
    gd.g.directed();

    // setup link capacity storage
    int maxLinkCount = cItem::count() * cItem::count();
    const int infinite = 2e9;
    gd.edgeWeight.resize(maxLinkCount, infinite);

    for (cHold *hold : cHold::get())
    {
        if (hold->count() > 1)
        {
            // source
            // create links from holdings with spare items into trading graph

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
            // create links from trading graph to empty holdings

            int ie = gd.g.add(
                hold->name(),
                "sink_" + hold->name());
            gd.edgeWeight[ie] = 1;
            gd.g.add(
                "sink_" + hold->name(),
                "all_sink");
        }
    }

    // create the trading links
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

    // Apply Edmonds-Karp algorithm using Pathfinder method
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
            if (flow(link) != 1)
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
                      << " times " << flow(link)
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
