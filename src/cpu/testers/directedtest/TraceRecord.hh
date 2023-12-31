#ifndef TRACERECORD_HH
#define TRACERECORD_HH

#include <string>
#include <iostream>
#include <fstream>

#include "base/logging.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/DirectedTest.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

class TraceRecord
{
public:
    // Constructors
    TraceRecord(int cpu_id, const Addr &data_addr,
                const Addr &pc_addr,
                std::string type);
    TraceRecord()
    {
        m_cpu_idx = 0;
        m_time = Cycles(0);
        m_data_address = 0;
        m_pc_address = 0;
        m_cmd = MemCmd::ReadReq;
    }

    // Destructor
    //  ~TraceRecord();

    // Public copy constructor and assignment operator
    TraceRecord(const TraceRecord &obj);
    TraceRecord &operator=(const TraceRecord &obj);

    // Public Methods
    bool node_less_then_eq(const TraceRecord &rec) const
    {
        return (this->m_time <= rec.m_time);
    }
    void print() const;
    bool input(std::istream &in);

public:
    // Private Methods

    // Data Members (m_ prefix)
    // Int cpu port
    int m_cpu_idx;
    Cycles m_time;
    Addr m_data_address;
    Addr m_pc_address;
    Packet::Command m_cmd;
};

inline extern bool node_less_then_eq(const TraceRecord &n1,
                                     const TraceRecord &n2);

// ******************* Definitions *******************

inline extern bool node_less_then_eq(const TraceRecord &n1,
                                     const TraceRecord &n2)
{
    return n1.node_less_then_eq(n2);
}


#endif //TRACERECORD_HH
