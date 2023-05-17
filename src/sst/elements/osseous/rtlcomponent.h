// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLE_VECTORSHIFTREG_H
#define _SIMPLE_VECTORSHIFTREG_H

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/timeConverter.h>
#include <queue>

//Header file will be changed to the RTL C-model under test
#include "rtl_header_riscv_mini.h"
#include "rtl_header_riscv_mini1.h"
#include <sst/core/link.h>
#include <sst/core/clock.h>
#include <sst/core/eli/elementinfo.h>
#include "rtlevent.h"
#include "arielrtlev.h"
#include "rtlreadev.h"
#include "rtlwriteev.h"
#include "rtlmemmgr.h"
#include "rtl_header_parent.h"

#undef UNLIKELY
#include "AXI_port.h"

struct mm_rresp_t
{
  uint64_t id;
  std::vector<char> data;
  bool last;

  mm_rresp_t(uint64_t id, std::vector<char> data, bool last)
  {
    this->id = id;
    this->data = data;
    this->last = last;
  }

  mm_rresp_t()
  {
    this->id = 0;
    this->last = false;
  }
};

namespace SST {
    namespace RtlComponent {
    
class Rtlmodel : public SST::Component {

public:
    //AXI structs

	Rtlmodel( SST::ComponentId_t id, SST::Params& params );
    ~Rtlmodel();

	void setup();
	void init(unsigned);
	void finish();

	bool clockTick( SST::Cycle_t currentCycle );

	SST_ELI_REGISTER_COMPONENT(
		Rtlmodel,
		"rtlcomponent",
		"Rtlmodel",
		SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
		"Demonstration of an External Element for SST",
		COMPONENT_CATEGORY_PROCESSOR
	)
    //Stats needs to be added. Now, stats will be added based on the outputs as mentioned by the user based on the RTL login provided. 
    SST_ELI_DOCUMENT_STATISTICS(
        { "read_requests",        "Statistic counts number of read requests", "requests", 1},   // Name, Desc, Enable Level
        { "write_requests",       "Statistic counts number of write requests", "requests", 1},
        { "read_request_sizes",   "Statistic for size of read requests", "bytes", 1},   // Name, Desc, Enable Level
        { "write_request_sizes",  "Statistic for size of write requests", "bytes", 1},
        { "split_read_requests",  "Statistic counts number of split read requests (requests which come from multiple lines)", "requests", 1},
        { "split_write_requests", "Statistic counts number of split write requests (requests which are split over multiple lines)", "requests", 1},
	    { "flush_requests",       "Statistic counts instructions which perform flushes", "requests", 1},
        { "fence_requests",       "Statistic counts instructions which perform fences", "requests", 1}
    )
    //Parameters will mostly be just frequency/clock in the design. User will mention specifically if there could be other parameters for the RTL design which needs to be configured before runtime.  Don't mix RTL input/control signals with SST parameters. SST parameters of RTL design will make the RTL design/C++ model synchronous with rest of the SST full system.   
	SST_ELI_DOCUMENT_PARAMS(
		{ "ExecFreq", "Clock frequency of RTL design in GHz", "1GHz" },
		{ "maxCycles", "Number of Clock ticks the simulation must atleast execute before halting", "1000" },
        {"memoryinterface", "Interface to memory", "memHierarchy.standardInterface"}
	)

    //Default will be single port for communicating with Ariel CPU. Need to see the requirement/use-case of multi-port design and how to incorporate it in our parser tool.  
    SST_ELI_DOCUMENT_PORTS(
        {"ArielRtllink", "Link to the Rtlmodel", { "Rtlmodel.RTLEvent", "" } },
        {"RtlCacheLink", "Link to Cache", {"memHierarchy.memInterface" , ""} }
    )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"memmgr", "Memory manager to translate virtual addresses to physical, handle malloc/free, etc.", "SST::RtlComponent::RtlMemoryManager"},
            {"memory", "Interface to the memoryHierarchy (e.g., caches)", "SST::Interfaces::StandardMem" }
    )

    void generateReadRequest(bool isAXIReadRequest, RtlReadEvent* rEv);
    void generateWriteRequest(RtlWriteEvent* wEv);
    void setDataAddress(uint8_t*);
    uint8_t* getDataAddress();
    void setBaseDataAddress(uint8_t*);
    uint8_t* getBaseDataAddress();

    //AXI APIs
    bool AXI_ar_ready() { return true; }
    bool AXI_aw_ready() { return !store_inflight; }
    bool AXI_w_ready() { return store_inflight; }
    bool AXI_b_valid() { return !bresp.empty(); }
    uint64_t AXI_b_resp() { return 0; }
    uint64_t AXI_b_id() { return AXI_b_valid() ? bresp.front() : 0; }
    bool AXI_r_valid() { return !rresp.empty(); }
    uint64_t AXI_r_resp() { return 0; }
    uint64_t AXI_r_id() { return AXI_r_valid() ? rresp.front().id: 0; }
    void * AXI_r_data() { return AXI_r_valid() ? &rresp.front().data[0] : &dummy_data[0]; }
    bool AXI_r_last() { return AXI_r_valid() ? rresp.front().last : false; }
    void AXI_tick( bool reset, 
               bool ar_valid, uint64_t ar_addr, uint64_t ar_id, uint64_t ar_size, uint64_t ar_len,
               bool aw_valid, uint64_t aw_addr, uint64_t aw_id, uint64_t aw_size, uint64_t aw_len,
               bool w_valid, uint64_t w_strb, void *w_data, bool w_last, 
               bool r_ready, 
               bool b_ready );
  void AXI_write(uint64_t addr, char *data);
  void AXI_write(uint64_t addr, char *data, uint64_t strb, uint64_t size);
  void AXI_read(uint64_t addr);

  void cpu_mem_tick(bool verbose, bool done_reset);

private:
	SST::Output output;
    
    //RTL Clock frequency of execution and maximum Cycles/clockTicks for which RTL simulation will run.
    std::string RTLClk;
	SST::Cycle_t maxCycles;
    int rtlheaderID;
 //   bool isPageTableReceived;
    bool isFirst;
    int in_temp_inp_size;
    int in_temp_count;

    int temp_inp_size;
    int temp_count;

    int mcnt;
    int mcnt1;
    uint64_t main_time;

    //SST Links
    SST::Link* ArielRtlLink;
    Interfaces::StandardMem* cacheLink;

    void handleArielEvent(SST::Event *ev);
    void handleMemEvent(Interfaces::StandardMem::Request* event);
    void handleAXISignals(uint8_t);
    void commitReadEvent(bool isAXIReadRequest, const uint64_t address, const uint64_t virtAddr, const uint32_t length);
    void commitWriteEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length, const uint8_t* payload);
    void sendArielEvent();
    uint64_t* getAXIDataAddress();
    
    TimeConverter* timeConverter;
    Clock::HandlerBase* clock_handler;
    bool writePayloads;
    bool update_registers, verbose, done_reset, sim_done;
    bool update_inp, update_ctrl, update_eval_args;
    RTLEvent ev;
    mRtlComponentParent *dut;
    AXITop *axiport;
    ArielComponent::ArielRtlEvent* RtlAckEv;
    uint64_t inp_VA, ctrl_VA, updated_rtl_params_VA, inp_PA, ctrl_PA, updated_rtl_params_PA;
    size_t inp_size, ctrl_size, updated_rtl_params_size;
    std::queue<char> cmd_queue;
    void* inp_ptr = nullptr;
    void* updated_rtl_params = nullptr;
    RtlMemoryManager* memmgr;
    bool mem_allocated = false;
    uint64_t sim_cycle;

    //AXI Handler signals
    uint64_t axi_tdata_$old = 0, axi_tdata_$next = 0;
    uint8_t axi_tvalid_$old = 0, axi_tvalid_$next = 0;
    uint8_t axi_tready_$old = 0, axi_tready_$next = 0;
    uint64_t axi_fifo_enq_$old = 0, axi_fifo_enq_$next = 0;
    uint64_t fifo_enq_$old = 0, fifo_enq_$next = 0;
    uint64_t fifo_deq_$old = 0, fifo_deq_$next = 0;

    std::unordered_map<Interfaces::StandardMem::Request::id_t, Interfaces::StandardMem::Request*>* pendingTransactions;
    std::unordered_map<Interfaces::StandardMem::Request::id_t, Interfaces::StandardMem::Request*>* AXIReadPendingTransactions;
    std::unordered_map<uint64_t, uint64_t> VA_VA_map;
    uint32_t pending_transaction_count;

    bool isStalled;
    uint64_t cacheLineSize;
    uint8_t *dataAddress, *baseDataAddress;
    uint64_t *AXIdataAddress; 
    
    Statistic<uint64_t>* statReadRequests;
    Statistic<uint64_t>* statWriteRequests;
    Statistic<uint64_t>* statFlushRequests;
    Statistic<uint64_t>* statFenceRequests;
    Statistic<uint64_t>* statReadRequestSizes;
    Statistic<uint64_t>* statWriteRequestSizes;
    Statistic<uint64_t>* statSplitReadRequests;
    Statistic<uint64_t>* statSplitWriteRequests;

    uint64_t tickCount;
    uint64_t dynCycles; 

    char *tempptr;
    char *bin_ptr;
    bool isRead;
    bool isLoaded;
    bool isWritten;
    bool canStartRead;
    bool isRespReceived;

    //AXI variables
    char* data;
    size_t size;
    size_t word_size;

    bool store_inflight;
    uint64_t store_addr;
    uint64_t store_id;
    uint64_t store_size;
    uint64_t store_count;
    std::vector<char> dummy_data;
    std::queue<uint64_t> bresp;

    std::queue<mm_rresp_t> rresp;

    uint64_t cycle;

    uint64_t curr_ar_id;

    unsigned int mCycles;
    uint64_t mBaseAddr;


};

 } //namespace RtlComponent 
} //namespace SST
#endif //_SIMPLE_VECTORSHIFTREG_H
