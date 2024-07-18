#include "INI.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <algorithm>

#include <vector>
#include <stdexcept> // std::invalid_argument
#include <limits> // std::numeric_limits

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
			std::wstring stringV;
		};

		struct INISection
		{
			std::unordered_map<std::wstring, INIEntry> entries;

			void addInt(const std::wstring& name, const std::wstring& asString, int64_t asInt)
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
			std::unordered_map<std::wstring, INISection> sections;
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

		static std::wstring TrimLeadingAndTrailingWhitespace(const std::wstring& str, const std::wstring& whitespace = L" \t")
		{
			const auto strBegin = str.find_first_not_of(whitespace);
			if (strBegin == std::wstring::npos)
			{
				return L""; // no content
			}

			const auto strEnd = str.find_last_not_of(whitespace);
			const auto strRange = strEnd - strBegin + 1;

			return str.substr(strBegin, strRange);
		}

		static unsigned int GetNumSubstringInstances(const std::wstring& str, const std::wstring& findSubstring)
		{
			unsigned int numInstances = 0;
			size_t pos = 0;
			while((pos = str.find(findSubstring, pos)) != std::wstring::npos )
			{
				++pos;
				++numInstances;
			}
			return numInstances;
		}


		bool CouldBeInt(const std::wstring& text)
		{
			if (text.empty())
			{
				return false;
			}
			if (text.find_first_not_of(L"-0123456789") != std::wstring::npos)
			{
				return false;
			}

			// If there is a minus sign, there can only be one
			// and it can only be at the start.
			const auto numMinusSigns = GetNumSubstringInstances(text, L"-");
			if (numMinusSigns > 1)
			{
				return false;
			}
			if ((numMinusSigns == 1) && (text[0] != '-'))
			{
				return false;
			}

			return true;
		}

		bool CouldBeDouble(const std::wstring& text)
		{
			if (text.empty())
			{
				return false;
			}
			if (text.find(' ') != std::wstring::npos)
			{
				return false;
			}
			if (text.find_first_not_of(L"-0123456789.eE") != std::wstring::npos)
			{
				return false;
			}

			// If there is a minus sign, there can only be one or two
			// and one can only be at the start.
			const auto numMinusSigns = GetNumSubstringInstances(text, L"-");
			if (numMinusSigns > 2)
			{
				return false;
			}
			if ((numMinusSigns > 0) && (text[0] != '-'))
			{
				return false;
			}

			return true;
		}

		// PHP's explode function in C++
		std::vector<std::wstring> explode(const std::wstring& delim, const std::wstring& str, const std::size_t limit = std::numeric_limits<std::size_t>::max())
		{
			if (delim.empty())
			{
				throw std::invalid_argument("Delimiter cannot be empty!");
			}
			std::vector<std::wstring> ret;
			if (limit <= 0)
			{
				return ret;
			}
			std::size_t pos = 0;
			std::size_t next_pos = str.find(delim);
			if (next_pos == std::wstring::npos || limit == 1)
			{
				ret.push_back(str);
				return ret;
			}
			for (;;)
			{
				ret.push_back(str.substr(pos, next_pos - pos));
				pos = next_pos + delim.size();
				if (ret.size() >= (limit - 1) || std::wstring::npos == (next_pos = str.find(delim, pos)))
				{
					ret.push_back(str.substr(pos));
					return ret;
				}
			}
		}


		bool INIFile::loadFile(const std::wstring& filePath)
		{
			std::vector<std::wstring> lines;
			std::wifstream stream (filePath);

			if (!stream.is_open())
			{
				return false;
			}

			std::wstring thisLine = L"";
			while (std::getline(stream, thisLine))
			{
				thisLine = TrimLeadingAndTrailingWhitespace(thisLine);

				// omit blank lines
				if (thisLine.empty())
				{
					continue;
				}

				// omit all-comment lines
				if (thisLine[0] == ';')
				{
					continue;
				}

				// Strip text after a semicolon
				auto semicolonPos = thisLine.find(';');
				if (semicolonPos != std::wstring::npos)
				{
					thisLine = thisLine.substr(0, semicolonPos);
				}

				// Line is okay, add it
				lines.push_back(thisLine);
			}

			stream.close();
			return this->loadLines(lines);
		}


		bool INIFile::loadLines(const std::vector<std::wstring>& lines)
		{
			impl->sections.clear();
			INISection* currentSection = nullptr;

			for (const std::wstring& line : lines)
			{
				// Handle sections
				if (line[0] == '[')
				{
					auto lastCharPos = line.length() - 1;
					if (line[lastCharPos] == ']')
					{
						std::wstring section = line.substr(1, lastCharPos - 1);
						currentSection = &(this->impl->sections[section]);
					}
					continue; // Section line handled, do not attempt to parse as key/value
				}

				// Use a default section if necessary
				if (currentSection == nullptr)
				{
					currentSection = &(this->impl->sections[L"default"]); // operator[] default creates if not exist
				}

				// StringUtils::SplitString is overkill as we only want the first split.
				auto firstEqualsPos = line.find('=');
				if (firstEqualsPos == std::wstring::npos)
				{
					return false; // If no equals sign, then not a key-value pair
				}

				const std::wstring name  = TrimLeadingAndTrailingWhitespace(line.substr(0, firstEqualsPos));
				const std::wstring value = TrimLeadingAndTrailingWhitespace(line.substr(firstEqualsPos + 1, line.length() - firstEqualsPos));

				// Use stringstream to attempt conversion
				std::wistringstream stream(value);

				// First, attempt int
				int64_t intValue;
				if ((CouldBeInt(value)) && (stream >> intValue))
				{
					currentSection->addInt(name, value, intValue);
					continue;
				}

				// Okay, not int. Try double.
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

				// A boolean? It might be, if it's four characters.
				if (value.length() == 4)
				{
					auto boolString = value;
					std::transform(boolString.begin(), boolString.end(), boolString.begin(), ::tolower);
					if (boolString.compare(L"true") == 0)
					{
						INIEntry e;
						e.type = INIEntry::type_t::boolT;
						e.boolV = true;
						e.stringV = value;
						currentSection->entries[name] = std::move(e);
						continue;
					}
					else if (boolString.compare(L"false") == 0)
					{
						INIEntry e;
						e.type = INIEntry::type_t::boolT;
						e.boolV = false;
						e.stringV = value;
						currentSection->entries[name] = std::move(e);
						continue;
					}
				}

				// Okay, just a string then.
				INIEntry e;
				e.type = INIEntry::type_t::stringT;
				e.stringV = value;
				currentSection->entries[name] = std::move(e);
				continue; // redundant

			}


			return true;
		}

		/*bool INIFile::saveFile (const std::wstring& filePath)
		{
			std::ofstream stream (filePath);
			if (!stream.is_open())
			{
				return false;
			}

			for (const auto& s : impl->sections)
			{
				stream << "[" << s.first << "]\n";

				for (const auto& intItem : s.second.intItems)
				{
					stream << intItem.first << " = " << intItem.second << "\n";
				}

				for (const auto& dblItem : s.second.doubleItems)
				{
					stream << dblItem.first << " = " << dblItem.second << "\n";
				}

				for (const auto& boolItem : s.second.boolItems)
				{
					stream << boolItem.first << " = " << (boolItem.second ? "true" : "false") << "\n";
				}

				for (const auto& strItem : s.second.stringItems)
				{
					stream << strItem.first << " = " << strItem.second << "\n";
				}
			}

			stream.flush();
			stream.close();
			return true;
		}*/

		int64_t INIFile::getInteger(const std::wstring& section, const std::wstring& item, int64_t defaultVal, bool* wasRead) const
		{
			const auto& sectionIter = impl->sections.find(section);
			if (sectionIter == impl->sections.end())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			const auto& entries = (*sectionIter).second.entries;
			const auto& itemIter = entries.find(item);
			if (itemIter == entries.cend())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (itemIter->second.type != INIEntry::type_t::intT)
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (wasRead)
			{
				*wasRead = true;
			}
			return (*itemIter).second.intV;
		}

		std::vector<int> INIFile::getIntegers(const std::wstring& section, const std::wstring& item, std::vector<int> defaultVal, bool* wasRead) const
		{
			const auto& sectionIter = impl->sections.find(section);
			if (sectionIter == impl->sections.end())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			const auto& entries = (*sectionIter).second.entries;
			const auto& itemIter = entries.find(item);
			if (itemIter == entries.cend())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (itemIter->second.type != INIEntry::type_t::intT && itemIter->second.type != INIEntry::type_t::stringT)
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (wasRead)
			{
				*wasRead = true;
			}

			if ((*itemIter).second.intV == 0)
			{
				return {};
			}

			std::wstring value = (*itemIter).second.stringV;
			std::vector<std::wstring> v = explode(L",", value);
			std::vector<int> key;

			for (std::size_t i = 0; i < v.size(); ++i)
			{
				key.push_back(std::stoi(v[i]));
			}

			return key;
		}

		double INIFile::getDouble(const std::wstring& section, const std::wstring& item, double defaultVal, bool* wasRead) const
		{
			const auto& sectionIter = impl->sections.find(section);
			if (sectionIter == impl->sections.end())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			const auto& entries = (*sectionIter).second.entries;
			const auto& itemIter = entries.find(item);
			if (itemIter == entries.cend())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (itemIter->second.type != INIEntry::type_t::intT)
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (wasRead)
			{
				*wasRead = true;
			}
			return (*itemIter).second.intV;
		}

		const std::wstring& INIFile::getString(const std::wstring& section, const std::wstring& item, const std::wstring& defaultVal, bool* wasRead) const {
			const auto& sectionIter = impl->sections.find(section);
			if (sectionIter == impl->sections.end())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			const auto& entries = (*sectionIter).second.entries;
			const auto& itemIter = entries.find(item);
			if (itemIter == entries.cend())
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (itemIter->second.type != INIEntry::type_t::stringT)
			{
				if (wasRead)
				{
					*wasRead = false;
				}
				return defaultVal;
			}

			if (wasRead)
			{
				*wasRead = true;
			}
			return (*itemIter).second.stringV;
		}

//
//		// Setting
//
//		bool INIFile::setInteger (const std::wstring& section, const std::wstring& item, int64_t value)
//		{
//			impl->sections[section].intItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setDouble  (const std::wstring& section, const std::wstring& item, double value)
//		{
//			impl->sections[section].doubleItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setBool    (const std::wstring& section, const std::wstring& item, bool value)
//		{
//			impl->sections[section].boolItems[item] = value;
//			return true;
//		}
//
//		bool INIFile::setString  (const std::wstring& section, const std::wstring& item, const std::wstring& value)
//		{
//			impl->sections[section].stringItems[item] = value;
//			return true;
//		}

	} // namespace io
} // namespace handy
