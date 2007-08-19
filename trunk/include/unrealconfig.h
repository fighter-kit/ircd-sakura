/*
 *   Unreal Internet Relay Chat Daemon
 *   (C) 2007 Daniel King (thedan_ at hotmail dot com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __UNREALCONFIG_H__
#define __UNREALCONFIG_H__

#include <sstream>
#include <list>
#include <fstream>
#include <iostream>
#include "configreader.h"

/** The Section class represents a section in an Unreal3-style configuration file.
 * A Section has a name attribute (which is compulsory), an optional value
 * attribute. Also optional are one or more sub-sections which are children to the
 * Section. Using subsections allows you to organise Sections in a graph.
 */
class CoreExport Section
{
protected:
	/** The name of the section
	 */
	std::string key;

	/** The value of the section
	 */
	std::string value;

	/** List of all child sections
	 */
	std::list<Section> subSections;

public:
	Section(){}

	/** Constructs and initialises a Section with the specified name but no
	 * value or child Sections.
	 * @param The name of the Section
	 */
	Section(const std::string &Key);

	/** Constructs and initialises a Section with the specified name and value
	 * but no child Sections.
	 * @param The name of the Section
	 * @param The value of the Section
	 */
	Section(const std::string &Key, const std::string &Value);

	/** Constructs and initialises a Section which is a deep copy of the
	 * specified block. This performs a deep copy so all child Sections 
	 * from the Section specified are copied as well.
	 * @param The Section to copy.
	 */
	Section(const Section &section);

	virtual ~Section(){}

	/** Sets the specified Section as a child Section of the invoked Section.
	 * Be careful and don't set a parent Section as a child otherwise you could
	 * get into a loop if you were to iterate through.
	 * @param The block to add as a child Section.
	 */
	void add(const Section &section);

	/** Removes the specified Section from the child Section list.
	 * If the Section specified is not currently a child of the Section
	 * then no action is taken.
	 * @param The Section to remove from the child Sections list.
	 */
	void remove(const Section &section);

	/** Returns the name attribute of the Section.
	 * @return The name of the Section.
	 */
	const std::string &getKey() const { return key; }

	/** Returns the value attribute of the Section. If the Section
	 * has no value then the returned string is an empty string.
	 * @return The value of the Section.
	 */
	const std::string &getValue() const { return value; }

	/** Returns a reference to the list of child Sections. If there are
	 * no child Sections then the returned list is empty.
	 * @return A reference to the list of child Sections.
	 */
	const std::list<Section> &getChildSections() const { return subSections; }

	/** Searches for a nested section/subsection with the key specified. The
	 * index parameter is used to return a specific occurrance of the section
	 * if there are multiple sections with the same key.
	 * @param The destination Section to copy the section to
	 * @param The key to search for
	 * @param The zero-based index of which occurrance to return
	 * @return true if the section at the index was found, false otherwise
	 */
	bool getSection(Section &dest, const std::string &key, unsigned index) const;

	/** Sets the block to be identical to the specified block.
	 * This operation performs a deep copy so each child block of 
	 * the given block are also copied.
	 * This should be avoided if the Section is likely to have many
	 * levels below it in the hierarchy.
	 * @param The Section to copy.
	 */
	Section &operator =(const Section &section);

	/** Performs an equality test. Returns true if the given Section has
	 * the same name, value and all child nodes occur of the Section have
	 * an identical Section as a child in the given Section. Returns false otherwise.
	 * The child block lists are order-independant so this operator will return
	 * true if the child Sections are identical, but arranged in a different
	 * order in the other's list.
	 * @param The Section to compare to.
	 */
	bool operator ==(const Section &section) const;

	/** Performs an equality test to check if the Section is NOT identical
	 * to the given Section. Returns false if the Section is identical to the
	 * given Section and false otherwise.
	 * @param The block to compare to.
	 */
	bool operator !=(const Section &section) const;
};

// -------------------------------------------------------------------------------------
// U4ServerConfig
// -------------------------------------------------------------------------------------

enum PARSE_RESULT { PARSE_ERROR = 0,	// Indicates an error occurred during parsing
					PARSE_OK,			// Parsing was successful
					PARSE_WARNING,  	// Parsing was successful, but some warnings occurred
					PARSE_IGNORED };	// Used by parseSection: Indicates the block was ignored

/** The U4ServerConfig class extends from the ServerConfig class and implements
 * a different parser which is capable of parsing Unreal3-style configuration
 * files. The class overrides the existing parser by overriding the LoadConf
 * member functions which do the parsing of the files.
 *
 * Config files are firstly parsed into the configSections attribute of the
 * and then are converted into the ConfigDataHash target of the LoadConf
 * function. This is because the unreal3-style grammar supports nested blocks,
 * whereas the InspIRCd parser does not. When converting from a Section graph to
 * a ConfigDataHash, any nested Sections beyond the first nest are ignored, but
 * remain in the configSections attribute which can be accessed later.
 *
 * This class does not need to do any additional processing, such as parsing
 * data from the ConfigDataHash into the attributes of the base class (ServerConfig)
 * since that is handled by the base class after the config file has been parsed.
 */
class CoreExport U4ServerConfig : public ServerConfig
{
private:
	/** Opens the file with the given filename. Logs an error if the file cannot 
	 * be opened. If the file is successfully opened then the contents are read 
	 * and passed to the parse member function to be parsed.
	 * @param The filename of the file to parse.
	 * @return 
	 */
	PARSE_RESULT parseFile(const char *fileName, std::ostringstream &errstream);

	/** Attempts to parse the specified text to generate a list of Sections.
	 * The function strips the text of comments and ensures general syntactical
	 * correctness before using the parseSection function to actually parse each
	 * section and perform syntax checking per-section.
	 *
	 * @param The text to parse
	 * @return PARSE_OK if the file was parsed successfully, PARSE_ERROR if any
	 *         errors occurred or PARSE_WARNING if any warnings occurred.
	 */
	PARSE_RESULT parse(const std::string &text, std::ostringstream &errstream, const std::string &filename);

	/** Attempts to parse the specified text to generate a single Section.
	 * This function is recursive by calling itself for every subSection detected.
	 * This function assumes a certain degree of syntactical correctness:
	 *   - For every opening brace '{' there is a matching closing brace '}'
	 *   - The text given contains no comments
	 * This function can also return PARSE_IGNORE to indicate that a block was
	 * ignored and the "dest" parameter was not altered. Sections are ignored in
	 * cases such as when the text passed contained an empty block or is otherwise
	 * insignificant. Cases like these are logged as warnings.
	 *
	 * @param The Section instance to parse the text to
	 * @param The text to parse
	 * @param Used for logging. This is the number to start counting lines from.
	 * @return PARSE_OK if the file was parsed successfully, PARSE_ERROR if any
	 *         errors occurred, PARSE_WARNING if any warnings occurred or PARSE_IGNORE
	 *         if the block was ignored.
	 */
	PARSE_RESULT parseSection(std::ostringstream &errstream, Section &dest, const std::string &text, unsigned startLine, const std::string &filename);

	/** Transforms the Section graph into a format where the Section graph can then be
	 * transformed into a ConfigDataHash of a 1:1 relationship.
	 */
	void doTransformations(std::ostringstream &errorstream, const std::string &filename);

	/** Searches the Section graph for include files and parses them
	 * @param errorstream errorstream for it
	 * @return PARSE_ERROR if one of the includes didn't work, PARSE_OK if all worked
	*/
	PARSE_RESULT doIncludes(ConfigDataHash &target, std::ostringstream &errorstream, const std::string &filename);
	
protected:

	/** The hierarchy of Sections generated by parsing the config file.
	 */
	std::list<Section> configSections;

	/** Logs a warning message on the specified line with the given message.
	 * @param The stringstream to write the warning message to
	 * @param The line number the warning occurred at
	 * @param The warning message to output
	 */
	void Warn(std::ostringstream &errstream, unsigned line, const std::string &msg)
	{
		errstream << line << ": Warning: " << msg << std::endl;
	}

	/** Logs a warning message with the given message.
	 * @param The stringstream to write the warning message to
	 * @param The warning message to output
	 */
	void Warn(std::ostringstream &errstream, const std::string &msg)
	{
		errstream << "Warning: " << msg << std::endl;
	}

	/** Logs a warning message with the given message and associated with a filename
	 * @param The stringstream to write the warning message to
	 * @param The warning message to output
	 * @param The filename to associate the warning message with
	 */
	void Warn(std::ostringstream &errstream, const std::string &msg, const std::string &filename)
	{
		errstream << filename << ": Warning: " << msg << std::endl;
	}

	/** Logs a warning message on the specified line with the given message and 
	 * associated with a filename.
	 * @param The stringstream to write the warning message to
	 * @param The line number the warning occurred at
	 * @param The warning message to output
	 * @param The filename to associate the warning message with
	 */
	void Warn(std::ostringstream &errstream, unsigned line, const std::string &msg, const std::string &filename)
	{
		errstream << filename << ":(" << line << ") Warning: " << msg << std::endl;
	}

	/** Logs an error message on the specified line with the given message.
	 * @param The stringstream to write the error message to
	 * @param The line number the error occurred at
	 * @param The error message to output
	 */
	void Error(std::ostringstream &errstream, unsigned line, const std::string &msg)
	{
		errstream << line << ": Error: " << msg << std::endl;
	}

	/** Logs an error message with the given message.
	 * @param The stringstream to write the error message to
	 * @param The error message to output
	 */
	void Error(std::ostringstream &errstream, const std::string &msg)
	{
		errstream << "Error: " << msg << std::endl;
	}

	/** Logs an error message with the given message and associated with the filename
	 * @param The stringstream to write the error message to
	 * @param The error message to output
	 * @param The filename to associate the error message with
	 */
	void Error(std::ostringstream &errstream, const std::string &msg, const std::string &filename)
	{
		errstream << filename << ": Error: " << msg << std::endl;
	}

	/** Logs an error message on the specified line with the given message, and
	 * associated with the filename.
	 * @param The stringstream to write the error message to
	 * @param The line the error occurred on
	 * @param The error message to output
	 * @param The filename to associate the error message with
	 */
	void Error(std::ostringstream &errstream, unsigned line, const std::string &msg, const std::string &filename)
	{
		errstream << filename << ":(" << line << ") Error: " << msg << std::endl;
	}

public:
	/** Default constructor initialises base class
	 * @param 
	 */
	U4ServerConfig(InspIRCd *instance) : ServerConfig(instance) {}

	/** Destructor has nothing to do, only present for inheritance purposes
	 */
	virtual ~U4ServerConfig(){}

	/** Returns a reference to the Section hierarchy of the configuration file.
	 * This list is intended to only be read from. This follows the UnrealIRCd paradigm
	 * "Settings from IRCd configuration are considered permanent until rehash (with 
	 * removal from or altering in configuration)". Please don't cast away the const :)
	 * @return A graph of sections representing the hierarchy of the config file parsed.
	 */
	const std::list<Section> &GetSections() const { return configSections; }

	/** Opens and parses the config file with the specified file name into the target
	 * parameter. Parses the configuration file into the target ConfigDataHash and also
	 * the protected configSections attribute.
	 * @param The destination ConfigDataHash to store the parsed data in
	 * @param The filename of the source file to read from
	 * @param The stream to write error messages to
	 * @param true if the configSections attribute should be reset before parsing, false otherwise
	 * @return True if the parsing was successful, false otherwise.
	 */
	virtual bool LoadConf(ConfigDataHash &target, const char* filename, std::ostringstream &errorstream, bool clear);

	/** Opens and parses the config file with the specified file name into the target
	 * parameter. Parses the configuration file into the target ConfigDataHash and also
	 * the protected configSections attribute.
	 * @param The destination ConfigDataHash to store the parsed data in
	 * @param The filename of the source file to read from
	 * @param The stream to write error messages to
	 * @param true if the configSections attribute should be reset before parsing, false otherwise
	 * @return True if the parsing was successful, false otherwise.
	 */
	virtual bool LoadConf(ConfigDataHash &target, const std::string &filename, std::ostringstream &errorstream, bool clear)
	{
		return LoadConf(target, filename.c_str(), errorstream, clear);
	}

	/** Same as LoadConf(ConfigDataHash &target, const char* filename, std::ostringstream &errorstream, bool clear)
	 * By default, the configSections attribute is reset.
	 * @param The destination ConfigDataHash to store the parsed data in
	 * @param The filename of the source file to read from
	 * @param The stream to write error messages to
	 * @return True if the parsing was successful, false otherwise.
	 */
	virtual bool LoadConf(ConfigDataHash &target, const char *filename, std::ostringstream &errorstream)
	{
		// Clear configSections by default.
		return LoadConf(target, filename, errorstream, true);
	}
	
	/** Same as LoadConf(ConfigDataHash &target, const std::string &filename, std::ostringstream &errorstream, bool clear)
	 * By default, the configSections attribute is reset.
	 * @param The destination ConfigDataHash to store the parsed data in
	 * @param The filename of the source file to read from
	 * @param The stream to write error messages to
	 * @return True if the parsing was successful, false otherwise.
	 */
	virtual bool LoadConf(ConfigDataHash &target, const std::string &filename, std::ostringstream &errorstream)
	{
		// Clear configSections by default.
		return LoadConf(target, filename.c_str(), errorstream, true);
	}
};

#endif 
