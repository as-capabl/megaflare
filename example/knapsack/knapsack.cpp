#define __CL_ENABLE_EXCEPTIONS

// ナップザック問題を解く
// 参考：http://qiita.com/items/b6df73661b3293b93982

#include <CL/cl.hpp>

#include <iostream>
#include <thread>
#include <cstdlib>
#include <numeric>
#include <chrono>
#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp> 
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <megaflare/code.hpp>
#include <megaflare/platform.hpp>
#include <megaflare/host.hpp>
#include <megaflare/misc/runner.hpp>

namespace pfm = megaflare::platform;
namespace chrono = std::chrono;
namespace code = megaflare::code;
namespace host = megaflare::host;
namespace mtpl = megaflare::tuples;
namespace misc = megaflare::misc;
namespace po = boost::program_options;

using namespace pfm;

auto fill_zero = code::func(
    "fill_zero",
    code::returns<pfm::void_>()|
    code::param<code::global<int_>*>("a"),
    "int id = get_global_id(0);\n"
    "a[id] = 0;"
);


#if 0
// セオリー的には上の方が速そうなのだが、
// A8-3850 iGPU + ati_drivers-12.11では
// 下の方が3割ほど速い。謎。
auto step_dp = code::func(
    "step_dp",
    code::returns<pfm::void_>()|
    code::param<code::global<int_>*>("aCurrent")|
    code::param<code::global<int_>*>("aNext")|
    code::param<int_>("iCap")|
    code::param<int_>("iPrice"),
    "int id = get_global_id(0);\n"
    "int v0 = aCurrent[id];"
    "if (iCap <= id) {"
    "  int v1 = aCurrent[id - iCap] + iPrice;"
    "  v0 = max(v0, v1);"
    "}"
    "aNext[id] = v0;"
);
#else
auto step_dp = code::func(
    "step_dp",
    code::returns<pfm::void_>()|
    code::param<code::global<int_>*>("aCurrent")|
    code::param<code::global<int_>*>("aNext")|
    code::param<int_>("iCap")|
    code::param<int_>("iPrice"),
    "int id = get_global_id(0);\n"
    "if (iCap <= id) {"
    " int v0 = aCurrent[id];"
    " int v1 = aCurrent[id - iCap] + iPrice;"
    " aNext[id] = max(v0, v1);"
    "}"
    "else {"
    " aNext[id] = aCurrent[id];"
    "}"
);
#endif


auto prog = code::program (
    code::kernel(step_dp),
    code::kernel(fill_zero)    
);

constexpr int item_count = 10;
constexpr int price_max = 10000;
constexpr int total_cap = 200;

struct item_type {
    int cap;
    int price;
private:
	friend class boost::serialization::access;
	template<class Archive>
    void serialize( Archive& ar, unsigned int ver )
    {
		ar & cap;
		ar & price;
    }
};

int
knapsack(misc::runner const & i_runner, host::program<decltype(prog)> i_program, std::vector<item_type> const & i_aItem, int i_nTotalCap)
{
    host::context const& context = i_runner.m_context;
    host::queue & queue = i_runner.m_queue;

    host::buffer<int_> buffers[2] = {
        {context, i_nTotalCap + 1},
        {context, i_nTotalCap + 1}
    };
    for(int i = 0; i < 2; ++i) {
        queue(
            run_kernel(
                i_program,
                fill_zero(buffers[i]),
                i_nTotalCap + 1
            )
        );
    }

    
    for(int i = 0; i < (int)i_aItem.size(); ++i) {
        queue(
            run_kernel(
                i_program, 
                step_dp(buffers[i % 2], 
                        buffers[(i+1) % 2], 
                        i_aItem[i].cap,
                        i_aItem[i].price),
                i_nTotalCap + 1
            )
        );
    }
    auto ret = 
        queue(
            buffers[i_aItem.size() % 2].with_range(
                [](cl_int* begin, cl_int* end) {
                    return *(end - 1);
                }
            )
        );
    return ret.get();
}

int knapsack_cpu(std::vector<item_type> const & i_aItem, int i_nTotalCap)
{
    std::vector<int> buffer(i_nTotalCap + 1);
    for (unsigned int j = 0; j < i_aItem.size(); ++j) {
        const item_type item = i_aItem[j];
        for (int i = i_nTotalCap; i >= item.cap; --i) {
            const int v0 = buffer[i];
            const int v1 = buffer[i - item.cap] + item.price;
            if (v1 > v0) {
                buffer[i] = v1;
            }
        }
    }
    return buffer.back();
}

void
test_run(misc::runner const & i_runner, 
         host::program<decltype(prog)> i_program)
{
    // small test
    std::vector<item_type> aItemSmall = {
        {10, 1},
        {20, 3},
    };
    int ret1 = knapsack(i_runner, i_program, aItemSmall, 10);
    assert(ret1 == 1);

    int ret2 = knapsack(i_runner, i_program, aItemSmall, 40);
    assert(ret2 == 1+3);

    int ret3 = knapsack(i_runner, i_program, aItemSmall, 20);
    assert(ret3 == 3);

    std::vector<item_type> aItemSingle = {
        {10, 5}
    };
    int retSingle = knapsack(i_runner, i_program, aItemSingle, 100);
    assert(retSingle == 5);

    int retSingle2 = knapsack(i_runner, i_program, aItemSingle, 5);
    assert(retSingle2 == 0);
}

std::vector<item_type>
random_item(int i_nItemCount)
{
    std::vector<item_type> aItem(i_nItemCount);
    std::srand(chrono::steady_clock::now().time_since_epoch().count());
    for (int i = 0; i < i_nItemCount; ++i) {
        aItem[i] = item_type{std::rand() % total_cap,
                             std::rand() % price_max};
    }
    return aItem;
}


int main(int i_argc, char** i_argv) try 
{
    int nItemCount = item_count;
    int iCap = total_cap;
    std::string sItemInFile;
    std::string sItemOutFile;

    po::options_description optDesc("Knapsack Options");
    optDesc.add_options()
        ("capacity,c", 
         po::value<int>(&iCap), 
         "Knapsack capacity.")
        ("item-count,n", 
         po::value<int>(&nItemCount), 
         "Item count.")
        ("test,t", "Run test.")
        ("cl", "Run OpenCL GPU code(default).")
        ("cl-cpu", "Run OpenCL CPU code.")
        ("no-cl", "Run non-OpenCL CPU code.")
        //("timer,x", "Time.")
        ("item-file-input,i", 
         po::value<std::string>(&sItemInFile), 
         "Input items from file.")
        ("item-file-output,o", 
         po::value<std::string>(&sItemOutFile), 
         "Output items to file.");
    
    po::variables_map values;
    
    try {
        po::store(
            po::parse_command_line(i_argc, i_argv, optDesc), 
            values
        );
        po::notify(values);


        // TODO: 以下の条件分岐は適当

        // GPU実行が指定されているか
        cl_device_type devType;
        bool bClRun = false;
        if(values.count("cl") || values.count("capacity")) {
            bClRun = true;
            devType = CL_DEVICE_TYPE_GPU;
        }
        // CPU実行が指定されているか
        if(values.count("cl-cpu")) {
            bClRun = true;
            devType = CL_DEVICE_TYPE_CPU;
        }

        misc::runner runner(devType);


        // アイテム構築
        std::vector<item_type> aItem;
        if(values.count("item-file-input")) {
            std::ifstream stream(sItemInFile);
            boost::archive::text_iarchive ia(stream);
            ia >> aItem;
            bClRun = true;
        }
        else if(values.count("item-count")) {
            aItem = random_item(nItemCount);
            bClRun = true;
        }

        // アイテム保存
        if(values.count("item-file-output")) {
            std::ofstream stream(sItemOutFile);
            boost::archive::text_oarchive oa(stream);

            // ワーニング消し
            // なぜ駄目かについてはobject trackingとrationareの説明を読め、とのこと
            oa << const_cast<decltype(aItem) const&>(aItem);
        }

        // 実行
        if(values.count("no-cl")) {
            std::cout << "Native code run" << std::endl;
            std::cout << aItem.size() << " items." << std::endl;
            std::cout << iCap << " of capacity." << std::endl;
            int ret = knapsack_cpu(aItem, iCap);
            std::cout << ret << std::endl;
        }
        else if(bClRun) {
            switch(devType) {
            case CL_DEVICE_TYPE_GPU:
                std::cout << "GPU run" << std::endl;
                break;
            case CL_DEVICE_TYPE_CPU:
                std::cout << "CPU run" << std::endl;
                break;
            }
            runner.with_program<decltype(prog)>(
                prog, 
                [aItem, iCap](misc::runner const & i_runner, 
                              host::program<decltype(prog)> i_program) 
                {
                    std::cout << aItem.size() << " items." << std::endl;
                    std::cout << iCap << " of capacity." << std::endl;
                    int ret = knapsack(i_runner, i_program, aItem, iCap);
                    std::cout << ret << std::endl;
                }
            );
        }
        else {
            runner.with_program<decltype(prog)>(prog, test_run);
        }

    }
    catch (...) {
        std::cerr << optDesc << std::endl;
        return -1;
    }

    return EXIT_SUCCESS;
}
catch (cl::Error err) {
    std::cerr << err.what() << "(" << err.err() << ")" ;
    return -1;
}
catch (misc::running_error err) {
    std::cerr << err;
    return -1;
}


