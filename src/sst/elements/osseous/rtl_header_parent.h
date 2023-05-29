#ifndef RTL_HEADER_PARENT_H
#define RTL_HEADER_PARENT_H
#include <uint.h>
#include <sint.h>

class mRtlComponentParent
{
    public:
    virtual void eval(bool update_registers, bool verbose, bool done_reset)
    {};
};
#endif