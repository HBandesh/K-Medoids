#include <omp.h>

#include <array>
#include <hpkmediods/types/parallelism.hpp>
#define BOOST_TEST_MODULE test_parallelism
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

namespace bdata = boost::unit_test::data;
using namespace hpkmediods;

std::array<Parallelism, 4> levels    = { Parallelism::Serial, Parallelism::OMP, Parallelism::MPI, Parallelism::Hybrid };
std::array<std::string, 4> strLevels = { SERIAL, OMP, MPI, HYBRID };

BOOST_DATA_TEST_CASE(test_parallelism_to_string, bdata::xrange(0, (int)levels.size()) ^ bdata::make(strLevels), level,
                     strLevel)
{
    BOOST_TEST(parallelismToString(levels[level]) == strLevel);
}

BOOST_DATA_TEST_CASE(test_parallelism_from_string, bdata::xrange(0, (int)levels.size()) ^ bdata::make(strLevels), level,
                     strLevel)
{
    auto isEqual = parallelismFromString(strLevel) == levels[level];  // boost can't handle class enums...stupid...
    BOOST_TEST(isEqual);
}
