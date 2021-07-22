#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace handy
{
    namespace util
    {
            class StringUtils
            {
                public:
                    static std::vector<std::string>& SplitString(const std::string& str, char delimeter, std::vector<std::string>& output)
                    {
                        std::stringstream sStream(str);
                        std::string item;
                        while(std::getline(sStream, item, delimeter))
                            output.push_back(item);
                        return output;
                    }

                    static std::vector<std::string> SplitString(const std::string& str, char delimeter)
                    {
                        std::vector<std::string> output;
                        SplitString(str,delimeter,output);
                        return output;
                    }

                    static unsigned int GetNumInstances(const std::string& str, const std::string& findSubstring)
                    {
                        unsigned int numInstances = 0;
                        size_t pos = 0;
                        while((pos = str.find(findSubstring, pos)) != std::string::npos )
                        {
                            ++pos;
                            ++numInstances;
                        }
                        return numInstances;
                    }


                    static std::string TrimLeadingAndTrailingWhitespace(const std::string& str, const std::string& whitespace = " \t")
                    {
                        const auto strBegin = str.find_first_not_of(whitespace);
                        if (strBegin == std::string::npos)
                            return ""; // no content

                        const auto strEnd = str.find_last_not_of(whitespace);
                        const auto strRange = strEnd - strBegin + 1;

                        return str.substr(strBegin, strRange);
                    }

                    //std::string reduce(const std::string& str, const std::string& fill = " ", const std::string& whitespace = " \t")
                    //{
                    //    // trim first
                    //    auto result = trim(str, whitespace);
                    //
                    //    // replace sub ranges
                    //    auto beginSpace = result.find_first_of(whitespace);
                    //    while (beginSpace != std::string::npos)
                    //    {
                    //        const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
                    //        const auto range = endSpace - beginSpace;
                    //
                    //        result.replace(beginSpace, range, fill);
                    //
                    //        const auto newStart = beginSpace + fill.length();
                    //        beginSpace = result.find_first_of(whitespace, newStart);
                    //    }
                    //
                    //    return result;
                    //}




                private:
                    StringUtils();
            };
    }

}
