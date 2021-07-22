#include "INI.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "StringUtils.hpp"

namespace handy
{
    namespace io
    {
    	struct INISection //: public util::NoCopy
    	{
    		std::unordered_map<std::string, int64_t>     intItems;
    		std::unordered_map<std::string, double>      doubleItems;
    		std::unordered_map<std::string, std::string> stringItems;
    		std::unordered_map<std::string, bool>        boolItems;
    	};

		struct INIFile::INIFileImpl : public util::NoCopy
        {
            std::unordered_map<std::string, INISection> sections;
            bool valid;
        };

		INIFile::INIFile()
		: impl(new INIFileImpl)
		{
			//
		}

		INIFile::~INIFile()
		{
			//
		}

        bool CouldBeInt(const std::string& text)
        {
            if (text.empty())
                return false;
            if (text.find_first_not_of("-0123456789") != std::string::npos)
                return false;

            // If there is a minus sign, there can only be one
            // and it can only be at the start.
            const auto numMinusSigns = util::StringUtils::GetNumInstances(text, "-");
            if (numMinusSigns > 1)
                return false;
            if ((numMinusSigns == 1) && (text[0] != '-'))
                return false;

            return true;
        }

		bool CouldBeDouble(const std::string& text)
        {
            if (text.empty())
                return false;
			if (text.find(' ') != std::string::npos)
				return false;
            if (text.find_first_not_of("-0123456789.eE") != std::string::npos)
                return false;

            // If there is a minus sign, there can only be one or two
            // and one can only be at the start.
            const auto numMinusSigns = util::StringUtils::GetNumInstances(text, "-");
            if (numMinusSigns > 2)
                return false;
            if ((numMinusSigns > 0) && (text[0] != '-'))
                return false;

            return true;
        }

		bool INIFile::loadFile(const std::string& filePath)
		{
			impl->sections.clear();
            std::vector<std::string> lines;
            std::ifstream stream (filePath);

            if (!stream.is_open())
                return false;

            std::string thisLine = "";
            while (std::getline(stream, thisLine))
            {
                thisLine = util::StringUtils::TrimLeadingAndTrailingWhitespace(thisLine);
                if (thisLine.empty())   continue; // omit blank lines
                if (thisLine[0] == ';') continue; // omit comments
                lines.push_back(thisLine);
            }

			INISection* currentSection = nullptr;

            for (const std::string& line : lines)
            {
            	// Handle sections
                if (line[0] == '[')
				{
					auto lastCharPos = line.length() - 1;
					if (line[lastCharPos] == ']')
					{
						std::string sectionName = line.substr(1, lastCharPos - 1);
						currentSection = &(impl->sections[sectionName]);
					}
					continue; // Section line handled, do not attempt to parse as key/value
				}

				// Make a default section if necessary
				if (currentSection == nullptr)
					currentSection = &(impl->sections["default"]); // operator[] default creates if not exist

				// StringUtils::SplitString is overkill as we only want the first split.
                auto firstEqualsPos = line.find('=');
                if (firstEqualsPos == std::string::npos)
					return false; // If no equals sign, then not a key-value pair

				const std::string name  = util::StringUtils::TrimLeadingAndTrailingWhitespace(line.substr(0, firstEqualsPos));
				const std::string value = util::StringUtils::TrimLeadingAndTrailingWhitespace(line.substr(firstEqualsPos + 1, line.length() - firstEqualsPos));

				// Use stringstream to attempt conversion
				std::istringstream stream(value);

				// First, attempt int
				int64_t intValue;
				if ((CouldBeInt(value)) && (stream >> intValue))
				{
					currentSection->intItems[name] = intValue;
					continue;
				}

				// Okay, not int. Try double
				double doubleValue;
				if ((CouldBeDouble(value)) && (stream >> doubleValue))
				{
					currentSection->doubleItems[name] = doubleValue;
					continue;
				}

				// A boolean?
				if (value.compare("true") == 0 || value.compare("TRUE") == 0)
				{
					currentSection->boolItems[name] = true;
					continue;
				}
				else if (value.compare("false") == 0 || value.compare("FALSE") == 0)
				{
					currentSection->boolItems[name] = false;
					continue;
				}

				// Okay, just a string then.
				currentSection->stringItems[name] = value;
				continue; // redundant





            }

//            std::cout << "INI parsing complete:\n";
//            for (const auto& s : impl->sections)
//			{
//				std::cout << "  Section " << s.first << ":\n";
//
//				std::cout << "    Int items:\n";
//				for (const auto& intItem : s.second.intItems)
//				{
//					std::cout << "      " << intItem.first << ":" << intItem.second << "\n";
//				}
//
//				std::cout << "    Double items:\n";
//				for (const auto& dblItem : s.second.doubleItems)
//				{
//					std::cout << "      " << dblItem.first << ":" << dblItem.second << "\n";
//				}
//
//				std::cout << "    Bool items:\n";
//				for (const auto& dblItem : s.second.boolItems)
//				{
//					std::cout << "      " << dblItem.first << ":" << dblItem.second << "\n";
//				}
//
//				std::cout << "    String items:\n";
//				for (const auto& strItem : s.second.stringItems)
//				{
//					std::cout << "      " << strItem.first << ":" << strItem.second << "\n";
//				}
//
//			}

			return true;
		}

		int64_t INIFile::getInteger   (const std::string& sectionName, const std::string& itemName, int64_t defaultValue) const
		{
			bool wasRead; // Shall ignore
			return this->getInteger(sectionName, itemName, defaultValue, wasRead);
		}

		int64_t INIFile::getInteger   (const std::string& sectionName, const std::string& itemName, int64_t defaultValue, bool& wasRead) const
		{
			const auto& sectionIter = impl->sections.find(sectionName);
			if (sectionIter == impl->sections.end())
			{
				wasRead = false;
				return defaultValue;
			}

			const auto& intItems = (*sectionIter).second.intItems;
			const auto& itemIter = intItems.find(itemName);
			if (itemIter == intItems.cend())
			{
				wasRead = false;
				return defaultValue;
			}

			wasRead = true;
			return (*itemIter).second;
		}

		double INIFile::getDouble (const std::string& sectionName, const std::string& itemName, double defaultValue) const
		{
			bool wasRead; // Shall ignore
			return this->getDouble(sectionName, itemName, defaultValue, wasRead);
		}

		double INIFile::getDouble (const std::string& sectionName, const std::string& itemName, double  defaultValue, bool& wasRead) const
		{
			const auto& sectionIter = impl->sections.find(sectionName);
			if (sectionIter == impl->sections.end())
			{
				wasRead = false;
				return defaultValue;
			}

			const auto& doubleItems = (*sectionIter).second.doubleItems;
			const auto& itemIter = doubleItems.find(itemName);
			if (itemIter == doubleItems.cend())
			{
				wasRead = false;
				return defaultValue;
			}

			wasRead = true;
			return (*itemIter).second;
		}

		bool INIFile::getBool (const std::string& sectionName, const std::string& itemName, bool defaultValue) const
		{
			bool wasRead; // Shall ignore
			return this->getBool(sectionName, itemName, defaultValue, wasRead);
		}

		bool INIFile::getBool (const std::string& sectionName, const std::string& itemName, bool  defaultValue, bool& wasRead) const
		{
			const auto& sectionIter = impl->sections.find(sectionName);
			if (sectionIter == impl->sections.end())
			{
				wasRead = false;
				return defaultValue;
			}

			const auto& boolItems = (*sectionIter).second.boolItems;
			const auto& itemIter = boolItems.find(itemName);
			if (itemIter == boolItems.cend())
			{
				wasRead = false;
				return defaultValue;
			}

			wasRead = true;
			return (*itemIter).second;
		}

		const std::string& INIFile::getString (const std::string& sectionName, const std::string& itemName, const std::string& defaultValue) const
		{
			bool wasRead; // Shall ignore
			return this->getString(sectionName, itemName, defaultValue, wasRead);
		}
		const std::string& INIFile::getString (const std::string& sectionName, const std::string& itemName, const std::string&  defaultValue, bool& wasRead) const
		{
			const auto& sectionIter = impl->sections.find(sectionName);
			if (sectionIter == impl->sections.end())
			{
				wasRead = false;
				return defaultValue;
			}

			const auto& stringItems = (*sectionIter).second.stringItems;
			const auto& itemIter = stringItems.find(itemName);
			if (itemIter == stringItems.cend())
			{
				wasRead = false;
				return defaultValue;
			}

			wasRead = true;
			return (*itemIter).second;
		}

    }
}
