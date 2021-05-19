#pragma once

#include "utils/CSingleton.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <variant>

const std::unordered_map < std::string, size_t> types =
{
	{"char", sizeof(char), },
	{"uint8_t", sizeof(uint8_t), },
	{"int8_t", sizeof(int8_t)},
	{"uint16_t", sizeof(uint16_t)},
	{"int16_t", sizeof(int16_t)},
	{"uint32_t", sizeof(uint32_t)},
	{"int32_t", sizeof(int32_t)},
	{"int", sizeof(int32_t)},
	{"uint64_t", sizeof(uint64_t)},
	{"int64_t", sizeof(int64_t)},
	{"float", sizeof(float)},
	{"double", sizeof(double)},
};

class ClassBase
{
public:
	ClassBase()
	{

	}
	ClassBase(std::string&& type_name_, bool is_pointer_, size_t array_size_, std::string&& name_, std::string&& desc_) :
		type_name(type_name_), is_pointer(is_pointer_), array_size(array_size_), name(name_), desc(desc_)
	{

	}
public:
	std::string type_name; /* TODO: improve this in the future, it's waste of memory */
	bool is_pointer = false;
	size_t array_size = std::numeric_limits<size_t>::max();  /* std::numeric_limits<size_t>::max() if isn't array */
	std::string name; /* variable name */
	std::string desc;  /* documentation */
};

class ClassElement : public ClassBase
{
public:
	ClassElement(std::string&& type_name_, bool is_pointer_, size_t array_size_, std::string&& name_, std::string&& desc_) :
		ClassBase(std::move(type_name_), is_pointer_, array_size_, std::move(name_), std::move(desc_))
	{

	}

	size_t GetSize()
	{
		size_t ret = 0;
		if(is_pointer)
			ret = pointer_size;
		else
			ret = types.at(type_name);

		if(array_size != std::numeric_limits<size_t>::max())
			ret *= array_size;
		return ret;
	}

	static size_t pointer_size;
private:

};


bool IsValidVariableType(std::string& type);


/*
Vector3
FirstStructure_t
- Vector3
SecondStructure_t

EmbeddedStruct
- FirstStructure_t
+++ Vector3
- SecondStructure_t
 */

class ClassContainer : public ClassBase /* holds a class and expands the further ones when a duplicated one found */
{
public:
	ClassContainer()
	{

	}
	ClassContainer(std::string type_name_, size_t packing_, bool is_pointer_ = 0, size_t array_size_ = std::numeric_limits<size_t>::max(), std::string&& name_ = "", std::string&& desc_ = "") :
		ClassBase(std::move(type_name_), is_pointer_, array_size_, std::move(name_), std::move(desc_)), packing(packing_)
	{

	}

	~ClassContainer()
	{
		/*
		for(auto* i : elems)
		{
			delete *elems;
		}
		*/
	}

	std::vector<std::variant<ClassElement*, ClassContainer*>> members;
	uint8_t type = 0; /* 0 = elemes, 1 = class */
	size_t packing = 0;
	//std::vector<ClassElement*> elems;
	ClassContainer* prev = nullptr;
	ClassContainer* next = nullptr;
	std::vector<ClassContainer*> classes;
};

class StructParser : public CSingleton < StructParser >
{
    friend class CSingleton < StructParser >;
public:
    void Init();
	void ParseStructure(std::string& input, std::string& output);

private:
	size_t FindPragmaPack(std::string& input, size_t from, size_t& push_start);
	size_t FindPragmaPop(std::string& input, size_t from, size_t to);
	size_t FindPacking(std::string& input, size_t from, size_t& push_start, size_t& pop_end);

	bool ParseElement(std::string& str_in, size_t& line_counter, ClassContainer* c);
	void GenerateOffsets(std::string& output, ClassContainer* c, ClassElement* e, size_t& offset);

	void DoCleanup();
	void TrimStructure(std::string& str_in, std::string& str_out);
	
	std::unordered_map<std::string, int32_t> definitions;
	std::vector<ClassContainer*> classes;  /* all classes in a container */

	ClassContainer* IsClassAlreadyExists(std::string& class_name)
	{
		for(auto& i : classes)
		{
			if(i->type_name == class_name)
				return i;
		}
		return NULL;
	}
};

