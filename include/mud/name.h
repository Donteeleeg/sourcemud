/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef MUD_NAME_H
#define MUD_NAME_H


class EntityArticleClass {
	public:
	typedef enum {
		NORMAL = 0,	// normal noun, like 'table' or 'priest':
					//	definite 'the', indefinte 'a'
		PROPER, 	// full proper name, like 'Sean' or 'Glamdring':
					//	no article
		UNIQUE, 	// unique non-proper, like the 'prince', the 'moon':
					//	'the' is always the article
					// also for proper nouns that need 'the', like the
					//  'Arctic Circle' or the 'King of England'
		PLURAL, 	// normal noun in plural form, like 'pants' or 'glasses':
					//	definite 'the', indefinite 'some'
		VOWEL,		// normal noun beginning with vowel, like 'emerald' or
					//  'adventurer': definite 'the', indefinite 'an'
		COUNT
	} type_t;
	
	public:
	inline EntityArticleClass (int s_value) : value((type_t)s_value) {}
	inline EntityArticleClass () : value(NORMAL) {}
	inline EntityArticleClass (const EntityArticleClass& s_value) : value(s_value.value) {}

	inline StringArg get_name() const { return names[value]; }
	inline type_t get_value () const { return value; }
	static EntityArticleClass lookup (StringArg name);

	inline bool operator == (const EntityArticleClass& dir) const { return dir.value == value; }
	inline bool operator != (const EntityArticleClass& dir) const { return dir.value != value; }

	private:
	type_t value;

	static String names[];
};

enum EntityArticleUsage {
	NONE = 0, // just the name, ma'am
	DEFINITE, // definite article: the
	INDEFINITE, // indefinite article: a or an
	YOUR, // 2nd-person singular/plural possessive
	MY, // 1st-person singular possessive
	OUR, // 1st-person plural possessive
	HIS, // 3rd-person singular masculine possessive
	HER, // 3rd-person singular feminine possessive
	ITS, // 3rd-person singular neuter possessive
	THEIR, // 3rd-person plural possessive
};

class EntityName {
	public:
	inline EntityName () : text(), article(EntityArticleClass::NORMAL) {}
	inline EntityName (EntityArticleClass s_article, StringArg s_text) :
		text(s_text), article(s_article) {}

	// these handle full names with articles
	String get_name () const;
	bool set_name (StringArg s_name); // returns false if it had to guess at the article

	// these handle name components
	inline String get_text () const { return text; }
	inline EntityArticleClass get_article () const { return article; }
	inline void set_text (StringArg s_text) { text = s_text; }
	inline void set_article (EntityArticleClass s_article) { article = s_article; }

	inline bool empty () const { return text.empty(); }

	private:
	String text;
	EntityArticleClass article;
};

class Entity;

// stream entity names
struct
StreamName {
	// constructors
	inline
	explicit StreamName(const Entity* s_ptr, EntityArticleUsage s_article = NONE, bool s_capitalize = false) :
		ref(*s_ptr), article(s_article), capitalize(s_capitalize) {}
	inline
	explicit StreamName(const Entity& s_ref, EntityArticleUsage s_article = NONE, bool s_capitalize = false) :
		ref(s_ref), article(s_article), capitalize(s_capitalize) {}

	friend const StreamControl& operator << (const StreamControl& stream, const StreamName& name);

	// data
	const Entity& ref; // the entity to print
	EntityArticleUsage article; // article type
	bool capitalize; // capitalize or not
};

#endif