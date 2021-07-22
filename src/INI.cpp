#include "INI.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <algorithm>

namespace handy
{
    namespace io
    {
    	struct INIEntry
    	{
    		enum class type_t {intT, doubleT, stringT, boolT} type;
    		union
    		{
    			int         intV;
    			double      doubleV;
    			bool        boolV;
    		};

    		// All values are also stored as strings.
			std::string stringV;
    	};

    	struct INISection
    	{
    		std::unordered_map<std::string, INIEntry> entries;

    		void addInt(const std::string& name, const std::string& asString, int64_t asInt)
    		{
				INIEntry e;
				e.type = INIEntry::type_t::intT;
				e.intV = asInt;
				e.stringV = asString;
				this->entries[name] = std::move(e);
    		}
    	};

		struct INIFile::INIFileImpl
        {
            std::unordered_map<std::string, INISection> sections;
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

		static std::string TrimLeadingAndTrailingWhitespace(const std::string& str, const std::string& whitespace = " \t")
		{
			const auto strBegin = str.find_first_not_of(whitespace);
			if (strBegin == std::string::npos)
				return ""; // no content

			const auto strEnd = str.find_last_not_of(whitespace);
			const auto strRange = strEnd - strBegin + 1;

			return str.substr(strBegin, strRange);
		}

		static unsigned int GetNumSubstringInstances(const std::string& str, const std::string& findSubstring)
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


        bool CouldBeInt(const std::string& text)
        {
            if (text.empty())
                return false;
            if (text.find_first_not_of("-0123456789") != std::string::npos)
                return false;

            // If there is a minus sign, there can only be one
            // and it can only be at the start.
            const auto numMinusSigns = GetNumSubstringInstances(text, "-");
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
            const auto numMinusSigns = GetNumSubstringInstances(text, "-");
            if (numMinusSigns > 2)
                return false;
            if ((numMinusSigns > 0) && (text[0] != '-'))
                return false;

            return true;
        }

        bool INIFile::loadFile(const std::string& filePath)
        {
            std::vector<std::string> lines;
            std::ifstream stream (filePath);

            if (!stream.is_open())
                return false;

            std::string thisLine = "";
            while (std::getline(stream, thisLine))
            {
                thisLine = TrimLeadingAndTrailingWhitespace(thisLine);

                // omit blank lines
                if (thisLine.empty())
					continue;

				// omit all-comment lines
                if (thisLine[0] == ';')
					continue;

				// Strip text after a semicolon
				auto semicolonPos = thisLine.find(';');
				if (semicolonPos != std::string::npos)
					thisLine = thisLine.substr(0, semicolonPos);

				// Line is okay, add it
                lines.push_back(thisLine);
            }

            stream.close();
            return this->loadLines(lines);

        }


		bool INIFile::loadLines(const std::vector<std::string>& lines)
		{
			impl->sections.clear();
			INISection* currentSection = nullptr;

            for (const std::string& line : lines)
            {
            	// Handle sections
                if (line[0] == '[')
				{
					auto lastCharPos = line.length() - 1;
					if (line[lastCharPos] == ']')
					{
						std::string section = line.substr(1, lastCharPos - 1);
						currentSection = &(this->impl->sections[section]);
					}
					continue; // Section line handled, do not attempt to parse as key/value
				}

				// Use a default section if necessary
				if (currentSection == nullptr)
					currentSection = &(this->impl->sections["default"]); // operator[] default creates if not exist

				// StringUtils::SplitString is overkill as we only want the first split.
                auto firstEqualsPos = line.find('=');
                if (firstEqualsPos == std::string::npos)
					return false; // If no equals sign, then not a key-value pair

				const std::string name  = TrimLeadingAndTrailingWhitespace(line.substr(0, firstEqualsPos));
				const std::string value = TrimLeadingAndTrailingWhitespace(line.substr(firstEqualsPos + 1, line.length() - firstEqualsPos));

				// Use stringstream to attempt conversion
				std::istringstream stream(value);

				// First, attempt int
				int64_t intValue;
				if ((CouldBeInt(value)) && (stream >> intValue))
				{
					currentSection->addInt(name, value, intValue);
					continue;
				}

//				 Okay, not int. Try double
				double doubleValue;
				if ((CouldBeDouble(value)) && (stream >> doubleValue))
				{
					INIEntry e;
					e.type = INIEntry::type_t::doubleT;
					e.intV = doubleValue;
					e.stringV = value;
					currentSection->entries[name] = std::move(e);
					continue;
				}
//
				// A boolean? It might be, if it's four characters
				if (value.length() == 4)
				{
					auto boolString = value;
					std::transform(boolString.begin(), boolString.end(), boolString.begin(), ::tolower);
					if (boolString.compare("true") == 0)
					{
						INIEntry e;
						e.type = INIEntry::type_t::boolT;
						e.boolV = true;
						e.stringV = value;
						currentSection->entries[name] = std::move(e);
						continue;
					}
					else if (boolString.compare("false") == 0)
					{
						INIEntry e;
						e.type = INIEntry::type_t::boolT;
						e.boolV = false;
						e.stringV = value;
						currentSection->entries[name] = std::move(e);
						continue;
					}
				}
//
				// Okay, just a string then.
				INIEntry e;
				e.type = INIEntry::type_t::stringT;
				e.stringV = value;
				currentSection->entries[name] = std::move(e);
				continue; // redundant

            }


			return true;
		}

//		bool INIFile::saveFile (const std::string& filePath)
//		{
//            std::ofstream stream (filePath);
//            if (!stream.is_open())
//                return false;
//
//            for (const auto& s : impl->sections)
//			{
//				stream << "[" << s.first << "]\n";
//
//				for (const auto& intItem : s.second.intItems)
//					stream << intItem.first << " = " << intItem.second << "\n";
//
//				for (const auto& dblItem : s.second.doubleItems)
//					stream << dblItem.first << " = " << dblItem.second << "\n";
//
//				for (const auto& boolItem : s.second.boolItems)
//					stream << boolItem.first << " = " << (boolItem.second ? "true" : "false") << "\n";
//
//				for (const auto& strItem : s.second.stringItems)
//					stream << strItem.first << " = " << strItem.second << "\n";
//			}
//
//			stream.flush();
//			stream.close();
//			return true;
//		}

		int64_t INIFile::getInteger   (const std::string& section, const std::string& item, int64_t defaultVal, bool* wasRead) const
		{
			const auto& sectionIter = impl->sections.find(section);
			if (sectionIter == impl->sections.end())
			{
				if (wasRead)
					*wasRead = false;
				return defaultVal;
			}

			const auto& entries = (*sectionIter).second.entries;
			const auto& itemIter = entries.find(item);
			if (itemIter == entries.cend())
			{
				if (wasRead)
					*wasRead = false;
				return defaultVal;
			}

			if (itemIter->second.type != INIEntry::type_t::intT)
			{
				if (wasRead)
					*wasRead = false;
				return defaultVal;
			}

			if (wasRead)
				*wasRead = true;
			return (*itemIter).second.intV;
		}

//
//		// Setting
//
//		bool INIFile::setInteger (const std::string& section, const std::string& item, int64_t value)
//		{
//			impl->sections[section].intItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setDouble  (const std::string& section, const std::string& item, double value)
//		{
//			impl->sections[section].doubleItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setBool    (const std::string& section, const std::string& item, bool value)
//		{
//			impl->sections[section].boolItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setString  (const std::string& section, const std::string& item, const std::string& value)
//		{
//			impl->sections[section].stringItems[item] = value;
//			return true;
//		}

    } // namespace io
} // namespace handy
