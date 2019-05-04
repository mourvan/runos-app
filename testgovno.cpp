#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>
#include <string>

int main()
{
    std::vector<std::string> tasks;
    boost::archive::text_oarchive oa(std::cout);
    oa << tasks;
}