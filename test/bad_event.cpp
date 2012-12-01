#include <cassert>
#include <CL/cl.hpp>

int main()
{
    cl_int err = CL_SUCCESS;

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.size() == 0) {
        return -1;
    }
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
    cl::Context context(CL_DEVICE_TYPE_GPU, properties);

    cl_event pEv = clCreateUserEvent(context(), &err);
    clReleaseEvent(pEv);

    assert(clRetainEvent(pEv) != CL_SUCCESS );
}
