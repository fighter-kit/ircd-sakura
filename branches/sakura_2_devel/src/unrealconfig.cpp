#include "unrealconfig.h"
#ifdef WIN32
#pragma warning(disable:4996)
#endif

Section::Section(const std::string &Key)
	:key(Key)
{}

Section::Section(const std::string &Key, const std::string &Value)
	:key(Key),
	value(Value)
{}

Section::Section(const Section &section)
	:key(section.key),
	value(section.value)
{
	for (std::list<Section>::const_iterator iter = section.subSections.begin();
		iter != section.subSections.end();
		++iter)
	{
		subSections.push_back(*iter);
	}
}

void Section::add(const Section &section)
{
	subSections.push_back(section);
}

void Section::remove(const Section &section)
{
	subSections.remove(section);
}

bool Section::getSection(Section &dest, const std::string &key, unsigned index) const
{
	unsigned count = 0;
	for (std::list<Section>::const_iterator iter = subSections.begin();
		 iter != subSections.end();
		 ++iter)
	{
		if (iter->getKey() == key)
		{
			++count;
			if (count-1 == index)
			{
				dest = *iter;
				return true;
			}
		}
	}
	return false;
}

Section &Section::operator =(const Section &section)
{
	key = section.key;
	value = section.value;

	subSections.clear();
	for (std::list<Section>::const_iterator iter = section.subSections.begin();
		iter != section.subSections.end();
		++iter)
	{
		subSections.push_back(*iter);
	}

	return *this;
}

bool Section::operator ==(const Section &section) const
{
	if (key != section.key || value != section.value)
		return false;

	if (subSections.size() != section.subSections.size())
		return false;

	bool exists;
	for (std::list<Section>::const_iterator iter = subSections.begin();
		 iter != subSections.end();
		 ++iter)
	{
		exists = false;

		/* Sections may occur in a different order in the other's subSection list,
		   should check through the whole list rather than just the same
	       position. */
		for (std::list<Section>::const_iterator iter2 = section.subSections.begin();
			 iter2 != section.subSections.end();
			 ++iter2)
		{
			if (*iter == *iter2)
			{
				exists = true;
				break;
			}
		}

		if (!exists)
			return false;
	}

	return true;
}

bool Section::operator !=(const Section &section) const
{
	return !(*this == section);
}

// -------------------------------------------------------------------------------------
// U4ServerConfig
// -------------------------------------------------------------------------------------

PARSE_RESULT U4ServerConfig::parse(const std::string &text, std::ostringstream &errstream, const std::string &filename)
{
	std::stringstream buffer;
	buffer << std::noskipws;
	char ch;
	unsigned currentLine = 1;
	bool hasWarnings = false;

	/* Strip comments and remove \r's then store in buffer */
	for (unsigned i = 0; i < text.length(); ++i)
	{
		ch = text.at(i);

		switch (ch)
		{
		case '\"':
			/* Store all text in quotes */
			buffer << ch;

			if (i+1 >= text.length())
				break;

			ch = text.at(++i);
			while (ch != '\"' && i < text.length()-1)
			{
				buffer << ch;
				ch = text.at(++i);
			}
			buffer << ch; /* terminating quote */
			break;

		case '#':
			if (i+1 >= text.length())
				break;

			ch = text.at(++i);
			while (i < text.length()-1 && ch != '\n')
				ch = text.at(++i);
			--i;
			break;

		case '/':
			if (i+1 >= text.length())
				break;

			ch = text.at(++i);
			if (ch == '/')
			{
				while(i < text.length()-1 && ch != '\n')
				{
					ch = text.at(++i);
				}
				--i;
			}
			/* Multiline comments */
			else if (ch == '*')
			{
				unsigned indent = 1; /* Keep track of nested comments */

				ch = text.at(++i);
				for (;ch != '\0' && i < text.length()-1; ch = text.at(++i))
				{
					if (ch == '*')
					{
						ch = text.at(++i);
						if (ch == '/')
						{
							--indent;
							if (indent == 0)
								break;
						}
						else
						{
							ch = text.at(--i);
						}
					}
					else if (ch == '/' && text.at(i+1) == '*')
					{
						++indent;
						++i;
					}
					else if (ch == '\n')
					{
						buffer << ch;	/* Preserve lines */
						++currentLine;
					}

					/* Prevent crashing (reading past end of the string) */
					if (i+1 >= text.length())
						break;
				}
				if (ch == '\0')
					--i;

				if (indent != 0)
				{
					Warn(errstream, currentLine, "Missing closing multiline comment at end of file", filename);
					hasWarnings = true;
				}
			}
			else
			{
				--i;
				buffer << '/';
			}
			break;

			/* Ignore \r's */
		case '\r':
			break;

		case '\n':
			++currentLine;
		default:
			buffer << ch;
			break;
		}
	}

	/*/  Parse blocks*/
	unsigned indent = 0;
	currentLine = 1;
	unsigned startLine;
	bool finishedSection;
	Section temp;

	while (!buffer.eof())
	{
		std::stringstream section;
		section << std::noskipws;

		/* Read any preceding whitespace */
		do
		{
			buffer >> ch;
			if (ch == '\n')
				currentLine++;
		}while((ch == ' ' || ch == '\n' || ch == '\t') && !buffer.eof());
		buffer.putback(ch);

		startLine = currentLine;

		finishedSection = false;
		do
		{
			buffer >> ch;

			if (buffer.eof())
				break;

			/* Watch for missing semicolons */
			if (finishedSection && ch != ';' && ch != ' ' && ch != '\n' && ch != '\t')
			{
				Error(errstream, currentLine, "Syntax Error: Missing closing semicolon for the previous block", filename);
				return PARSE_ERROR;
			}

			switch (ch)
			{
			case '{':
				++indent;
				section << ch;
				break;

			case '}':
				if (indent == 0)
				{
					Warn(errstream, currentLine, "Ignoring extra closing brace (did you forget a '{' somewhere above?)", filename);
					hasWarnings = true;
				}
				else
				{
					--indent;
					section << ch;
				}

				if (indent == 0)
					finishedSection = true;	/* Now expecting the terminating semicolon */

				break;

			case '\n':
				++currentLine;	/* Fall through */
			default:
				section << ch;
				break;
			}
		}while (!(ch == ';' && indent == 0));

		/* Make sure there is a matching } for every { */
		if (buffer.eof() && indent > 0)
		{
			std::stringstream err;
			err << "End of File reached while parsing the block on line ";
			err << startLine;
			err << ". Did you miss a closing '}' somewhere?";
			Error(errstream, currentLine, err.str(), filename);
			return PARSE_ERROR;
		}

		/* Avoid parsing nothingness */
		if (section.str().length() > 0)
		{
			configSections.push_back(Section());
			switch (parseSection(errstream, configSections.back(), section.str(), startLine, filename))
			{
			case PARSE_WARNING:
				hasWarnings = true;
			case PARSE_OK:
				break;

			case PARSE_ERROR:
				configSections.pop_back();
				return PARSE_ERROR;

			case PARSE_IGNORED:
				configSections.pop_back();
				break;
			}
		}
	}

	if (hasWarnings)
		return PARSE_WARNING;
	else
		return PARSE_OK;
}

PARSE_RESULT U4ServerConfig::parseSection(std::ostringstream &errstream, Section &dest, const std::string &text, unsigned startLine, const std::string &filename)
{
	std::stringstream key, value;
	Section temp;
	unsigned i = 0;
	unsigned warns = 0;
	char ch = text.at(i++);
	unsigned currentLine = startLine;
	std::size_t textLength = text.length();

	/* Read any preceding whitespace */
	while((ch == ' ' || ch == '\n' || ch == '\t') && i < textLength)
	{
		if (ch == '\n')
			currentLine++;
		ch = text.at(i++);
	}
	--i;

	/* Read the key */
	for (;i < textLength; i++)
	{
		ch = text.at(i);

		/* Treat anything between quotes as a single key */
		if (ch == '\"')
		{
			ch = text.at(++i);
			while (ch != '\"' && ch != '\n' && i < textLength-1)
			{
				key << ch;
				ch = text.at(++i);
			}
		}

		if (ch == ';' || ch == '{' || ch == ' ' || ch == '\n' || ch == '\t')
			break;
		else if (ch != '"')
			key << ch;
	}

	if (key.str().length() == 0)
	{
		Warn(errstream, currentLine, "Ignoring extra semicolon", filename);
		++warns;
		return PARSE_IGNORED;
	}

	/* Read any preceding whitespace */
	/* Here we also treat '=' as whitespace so that users can do:
	 * key = value; in the config file */
	while((ch == ' ' || ch == '\n' || ch == '\t' || ch == '=') && i < textLength)
	{
		if (ch == '\n')
			currentLine++;
		ch = text.at(i++);
	}
	--i;

	if (ch != ';' && ch != '{' && i != textLength)
	{
		/* Read the value */
		for (;i < textLength; i++)
		{
			ch = text.at(i);

			/* Treat anything between quotes as a single value */
			if (ch == '\"')
			{
				ch = text.at(++i);
				while (ch != '\"' && ch != '\n' && i < textLength-1)
				{
					value << ch;
					ch = text.at(++i);
				}
			}

			if (ch == ';' || ch == '{' || ch == ' ' || ch == '\n' || ch == '\t')
				break;
			else if (ch != '"')
				value << ch;
		}
	}

	/* Discard any whitespace, and here we also discard any = signs */
	while((ch == ' ' || ch == '\n' || ch == '\t' || ch == '=') && i < textLength)
	{
		if (ch == '\n')
			currentLine++;
		ch = text.at(++i);
	}

	/* Expecting a ; or { */
	if (ch != ';' && ch != '{')
	{
		Error(errstream, currentLine, "Syntax Error: Too many block values. Expected either ';' or '{'. Did you forget a ';' before this block?", filename);
		return PARSE_ERROR;
	}

	dest = Section(key.str(), value.str());

	/* Read any subSections */
	unsigned indent = 0;
	if (ch == '{' && i < textLength)
	{
		ch = text.at(++i);

		/* Discard whitespace ...again */
		while((ch == ' ' || ch == '\n' || ch == '\t') && i < textLength)
		{
			if (ch == '\n')
				currentLine++;
			ch = text.at(++i);
		}

		while (!(ch == '}' && indent == 0) && i < textLength)
		{
			std::stringstream subSection;

			while (!(ch == ';' && indent == 0) && i < textLength)
			{
				ch = text.at(i++);

				switch (ch)
				{
				case '{':
					++indent;
					subSection << ch;
					break;

				case '}':
					if (indent == 0)
					{
						//Error(errstream, currentLine, "Syntax Error: Extra closing brace or missing semicolon", filename);
						//return PARSE_ERROR;
						Warn(errstream, currentLine, "Ignoring extra '}'. Did you forget an opening '{' somewhere above?", filename);
					}
					else
					{
						--indent;
						subSection << ch;
					}
					break;

				case '\n':
					currentLine++;	/* Fall through */
				default:
					subSection << ch;
				}
			}

			/* Avoid parsing nothingness */
			if (subSection.str().length() > 0)
			{
				if (parseSection(errstream, temp, subSection.str(), currentLine, filename) == PARSE_ERROR)
					return PARSE_ERROR;
				else
					dest.add(temp);
			}

			ch = text.at(i++);

			/* Read any preceding whitespace */
			while((ch == ' ' || ch == '\n' || ch == '\t') && i < textLength)
			{
				if (ch == '\n')
					currentLine++;
				ch = text.at(i++);
			}
			--i;
		}
	}

	if (warns > 0)
		return PARSE_WARNING;
	else
		return PARSE_OK;
}

PARSE_RESULT U4ServerConfig::parseFile(const char *fileName, std::ostringstream &errstream)
{
	std::ifstream file(fileName, std::ios::in);
	if (!file.is_open())
	{
		Error(errstream, "Unable to open file", fileName);
		return PARSE_ERROR;
	}
	else
	{
		std::stringstream buffer;
		char ch;

		while (!file.eof())
		{
			file.read(&ch, 1);
			if (!file.eof())
				buffer << ch;
			else
				break;
		}

		return parse(buffer.str(), errstream, fileName);
	}
}

bool U4ServerConfig::LoadConf(ConfigDataHash &target, const char* filename, std::ostringstream &errorstream, bool clear)
{
	if (clear)
		configSections.clear();

	PARSE_RESULT result = parseFile(filename, errorstream);

	if (result != PARSE_ERROR)
	{
		if (doIncludes(target, errorstream, filename) == PARSE_ERROR)
			return false;
			
		doTransformations(errorstream, filename);
		target.clear();

		/* Read data from the sections into the ConfigDataHash target. Any 
		 * nested 'levels' deeper than the first nest is ignored, but remains in
		 * the configSections attribute
		 */
		for (std::list<Section>::const_iterator iter = configSections.begin();
			 iter != configSections.end();
			 ++iter)
		{
			KeyValList entries;

			/* Read child entries of the block */
			for (std::list<Section>::const_iterator subIter = iter->getChildSections().begin();
				 subIter != iter->getChildSections().end();
				 ++subIter)
			{
				entries.push_back(std::make_pair(subIter->getKey(), subIter->getValue()));
			}

			target.insert(std::make_pair(iter->getKey(), entries));
		}
		return true;
	}
	else
		return false;
}

void U4ServerConfig::doTransformations(std::ostringstream &errorstream, const std::string &filename)
{
	Section temp, temp2;
	std::list<Section>::iterator tempIter;

	for (std::list<Section>::iterator iter = configSections.begin();
		 iter != configSections.end();)
	{
		/* U4 style:    loadmodule "modulename";
		 * Change from: module { name "modulename" };
		 */
		if (iter->getKey() == "loadmodule")
		{
			if (iter->getValue().length() > 0)
			{
				temp = Section("module");
				temp2 = Section("name", iter->getValue());
				temp.add(temp2);
				configSections.push_back(temp);
			}
			else
				Warn(errorstream, "No module name specified on a loadmodule entry, ignoring", filename);
		}
		/* U4 style:    loadmodules { "module1"; "module2"; };
		 * Change from: module { name "module1"; };
		 *			    module { name "module2"; };
		 */
		else if (iter->getKey() == "loadmodules")
		{
			if (iter->getChildSections().size() > 0)
			{
				for (std::list<Section>::const_iterator modulesIter = iter->getChildSections().begin();
					 modulesIter != iter->getChildSections().end();
					 ++modulesIter)
				{
					if (modulesIter->getKey().length() > 0)
					{
						temp = Section("module");
						temp2 = Section("name", modulesIter->getKey());
						temp.add(temp2);
						configSections.push_back(temp);
					}
					else
						Warn(errorstream, "Ignoring empty module name in loadmodules", filename);
				}
			}
			else
				Warn(errorstream, "No modules specified in a loadmodules entry, ignoring", filename);
		}
		/* U4 style:    oper "Name" { ... };
		 * Change from: oper { name "Name"; ... };
		 */
		else if (iter->getKey() == "oper")
		{
			if (iter->getValue().length() > 0)
			{
				temp = Section("oper");
				temp2 = Section("name", iter->getValue());
				temp.add(temp2);
				for (std::list<Section>::const_iterator iter2 = iter->getChildSections().begin();
					 iter2 != iter->getChildSections().end();
					 ++iter2)
				{
					temp.add(*iter2);
				}
				configSections.push_front(temp);/* Insert at front to avoid going over
												   this Section again as we continue
												   through the list */

				/* Remove old entry. */
				/* Use tempIter to swap iters and ensure we 
				   can continue with a valid iterator */
				tempIter = iter;
				++tempIter;
				configSections.erase(iter);
				iter = tempIter;
				continue; /* Avoid incrementing the iterator again 
						   * (which are outside these if-else clauses) */
			}
		}
		/* U4 style:    link "Name" { ... };
		 * Change from: link { name "Name"; ... };
		 */
		else if (iter->getKey() == "link")
		{
			if (iter->getValue().length() > 0)
			{
				temp = Section("link");
				temp2 = Section("name", iter->getValue());
				temp.add(temp2);
				for (std::list<Section>::const_iterator iter2 = iter->getChildSections().begin();
					 iter2 != iter->getChildSections().end();
					 ++iter2)
				{
					temp.add(*iter2);
				}
				configSections.push_front(temp);/* Insert at front to avoid going over
												   this Section again as we continue
												   through the list */

				/* Remove old entry. */
				/* Use tempIter to swap iters and ensure we 
				   can continue with a valid iterator */
				tempIter = iter;
				++tempIter;
				configSections.erase(iter);
				iter = tempIter;
				continue; /* Avoid incrementing the iterator again 
						   * (which are outside these if-else clauses) */
			}
		}
		/* U4 style:    banlist "#Channel" { ... };
		 * Change from: banlist { chan "#Channel"; ... };
		 */
		else if (iter->getKey() == "banlist")
		{
			if (iter->getValue().length() > 0)
			{
				temp = Section("banlist");
				temp2 = Section("chan", iter->getValue());
				temp.add(temp2);
				for (std::list<Section>::const_iterator iter2 = iter->getChildSections().begin();
					 iter2 != iter->getChildSections().end();
					 ++iter2)
				{
					temp.add(*iter2);
				}
				configSections.push_front(temp);/* Insert at front to avoid going over
												   this Section again as we continue
												   through the list */

				/* Remove old entry. */
				/* Use tempIter to swap iters and ensure we 
				   can continue with a valid iterator */
				tempIter = iter;
				++tempIter;
				configSections.erase(iter);
				iter = tempIter;
				continue; /* Avoid incrementing the iterator again 
						   * (which are outside these if-else clauses) */
			}
		}

		++iter; /* Keep this here so that we can remove entries without invalidating the iterator */
	}
}

PARSE_RESULT U4ServerConfig::doIncludes(ConfigDataHash &target, std::ostringstream &errorstream, const std::string &filename)
{
	for (std::list<Section>::const_iterator iter = configSections.begin();
		 iter != configSections.end();
		 ++iter)
	{
		if (iter->getKey() == "include")
		{
			if (iter->getValue().length() > 0)
			{
				if (!LoadConf(target, iter->getValue().c_str(), errorstream, false))
					return PARSE_ERROR;
			}
			else
				Warn(errorstream, "No filename in include entry", filename);
		}
	}
	return PARSE_OK;
}
