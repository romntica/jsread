// jsread.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
// simple JSON parser : support only ascii code and integer number only

#include "stdafx.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <list>

using namespace std;


struct jsBase {
	virtual void printOut(int ts) = 0;
};

struct jsString : jsBase {
	string str_value;
	virtual void printOut(int ts) {
		cout << " \"" << str_value.c_str() << "\"";
	}
};

struct jsNumber : jsBase {
	long	num_value;
	virtual void printOut(int ts) {
		cout << " " << num_value;
	}
};

struct jsBoolean : jsBase {
	bool	bool_value;
	virtual void printOut(int ts) {
		if (bool_value) {
			cout << " true";
		}
		else {
			cout << " false";
		}
	}
};

struct jsArray : jsBase {
	list<jsBase*>	elements;
	~jsArray() {
		for (auto d : elements)
			delete d;
	}
	virtual void printOut(int ts) {
		cout << endl;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}
		cout << '[' << endl;
		ts++;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}

		for (auto d : elements) {
			d->printOut(ts);
			cout << ',';
		}
		cout << endl;
		ts--;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}
		cout << ']';
	}
};

struct jsObject : jsBase {
	list<jsBase *>	elements;
	~jsObject() {
		for (auto d : elements)
			delete d;
	}

	virtual void printOut(int ts) {
		cout << endl;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}
		cout << '{' << endl;
		ts++;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}

		for (auto d : elements) {
			d->printOut(ts);
			cout << ',';
		}
		cout << endl;
		ts--;
		for (int i = 0; i < ts; i++) {
			cout << '\t';
		}
		cout << '}';
	}
};

struct jsObjectElem : jsBase {
	string		key;
	jsBase		*value;
	virtual void printOut(int ts) {
		cout << " \"" << key.c_str() << "\":";
		value->printOut(ts);
	}
};

enum ePARSE_STATE {
	ePARSE_EMPTY, // val, arr, obj -> push EMPTY and goto VAL, ARR, OBJKEY 
	ePARSE_VALUE, // , -> pop / EOF -> pop
	ePARSE_ARRAY, // val, arr, obj -> push ARR and goto VAL, ARR, OBJKEY
	ePARSE_OBJKEY, // STR_VAL -> goto OBJVAL / } -> pop
	ePARSE_OBJVAL, // val -> pop 
	ePARSE_INVALID // error
};

enum eTOKEN_ACTION {
	eTOKEN_TO_COMMA,
	eTOKEN_TO_STR_VALUE,
	eTOKEN_TO_INT_VALUE,
	eTOKEN_TO_ARRAY,
	eTOKEN_TO_OBJKEY,
	eTOKEN_TO_CLOSE,
	eTOKEN_TO_INVALID
};

ePARSE_STATE transit_table[ePARSE_INVALID][eTOKEN_TO_INVALID] = {
	/*				COMMA				STR				INT				ARRARY			OBJKEY			CLOSE */
	/* EMPTY */{ ePARSE_INVALID,	ePARSE_VALUE,	ePARSE_VALUE,	ePARSE_ARRAY,	ePARSE_OBJKEY,	ePARSE_INVALID },
	/* VALUE */{ ePARSE_EMPTY,		ePARSE_INVALID,	ePARSE_INVALID,	ePARSE_INVALID,	ePARSE_INVALID,	ePARSE_INVALID },
	/* ARRAY */{ ePARSE_INVALID,	ePARSE_VALUE,	ePARSE_VALUE,	ePARSE_ARRAY,	ePARSE_OBJKEY,	ePARSE_EMPTY },
	/* OBJKEY */{ ePARSE_OBJKEY,	ePARSE_OBJVAL,	ePARSE_INVALID,	ePARSE_INVALID,	ePARSE_OBJKEY,	ePARSE_EMPTY },
	/* OBJVAL */{ ePARSE_INVALID,	ePARSE_OBJKEY,	ePARSE_OBJKEY,	ePARSE_ARRAY,	ePARSE_OBJKEY,	ePARSE_INVALID },
};

class jsM {
public:
	enum ePARSE_STATE		m_iCurState;		///< current parsing state
	list<enum ePARSE_STATE>	m_liState;			///< parsing state stack for Object and Array

	list<jsBase*>			m_liElement;		///< list for JSON value
	jsBase*					m_pCurElem;			///< current parsed JSON value

	list<list<jsBase*>*>	m_liContainer;		///< container stack for Object and Array
	list<jsBase*>*			m_pCurCont;			///< current container (start with m_liElement)

	istringstream			m_isIn;				///< input string stream

	string					last_str;			///< last parsed string token
	int						last_int;			///< last parsed integer token

	eTOKEN_ACTION getToken(void);
	ePARSE_STATE transitTo(enum eTOKEN_ACTION st);

	bool parse(void) {
		bool ret = true;
		enum eTOKEN_ACTION a;

		while (!m_isIn.eof()) {
			a = getToken();
			if (transitTo(a) == ePARSE_INVALID) {
				cout << "STOP PARSING - INVALID STATE" << endl;
				ret = false;
				break;
			}
		}
		return ret;
	}
	void print(void) {
		for (auto d : m_liElement) {
			d->printOut(0);
		}
	}

	jsM() {
		m_iCurState = ePARSE_EMPTY;
		m_pCurElem = nullptr;
		m_pCurCont = &m_liElement;
	}
	~jsM() {
		for (auto d : m_liElement)
			delete d;
	}
};

ePARSE_STATE jsM::transitTo(enum eTOKEN_ACTION a)
{
	enum ePARSE_STATE s = ePARSE_EMPTY;

	if (a == eTOKEN_TO_INVALID) {
		if (!m_liState.empty()) {
			s = m_liState.back();
			m_liState.pop_back();

			// discard current container
			delete m_pCurCont;

			m_pCurCont = m_liContainer.back();
			m_liContainer.pop_back();
		}
		else {
			// stop parsing.
		}
	}
	else if (a == eTOKEN_TO_CLOSE) {
		if (!m_liState.empty()) {
			s = m_liState.back();
			m_liState.pop_back();

			m_pCurCont = m_liContainer.back();
			m_liContainer.pop_back();
		}
		else {
			cout << "something wrong!! empty state list" << endl;
		}
	}
	else {
		s = transit_table[m_iCurState][a];
		if (a == eTOKEN_TO_ARRAY) {
			jsArray *a = new jsArray;
			if (m_iCurState == ePARSE_OBJVAL) {
				// curElem should be ObjKey
				((jsObjectElem *)m_pCurElem)->value = (jsBase*)a;
				m_iCurState = ePARSE_OBJKEY;
			}
			else {
				m_pCurElem = (jsBase *)a;
			}

			m_pCurCont->push_back(m_pCurElem);
			m_liState.push_back(m_iCurState);

			m_liContainer.push_back(m_pCurCont);
			m_pCurCont = &(a->elements);
		}
		else if (a == eTOKEN_TO_OBJKEY) {
			jsObject *o = new jsObject;
			if (m_iCurState == ePARSE_OBJVAL) {
				// curElem should be ObjKey
				((jsObjectElem *)m_pCurElem)->value = (jsBase*)o;
				m_iCurState = ePARSE_OBJKEY;
			}
			else {
				m_pCurElem = (jsBase *)o;
			}
			m_pCurCont->push_back(m_pCurElem);
			m_liState.push_back(m_iCurState);

			m_liContainer.push_back(m_pCurCont);
			m_pCurCont = &(o->elements);
		}
		else if (a == eTOKEN_TO_STR_VALUE) {
			if (m_iCurState == ePARSE_OBJKEY) {
				jsObjectElem *oe = new jsObjectElem;
				oe->key = last_str;
				m_pCurElem = (jsBase*)oe;
			}
			else {
				jsString *str = new jsString;
				str->str_value = last_str;
				if (m_iCurState == ePARSE_OBJVAL) {
					// curElem should be ObjKey
					((jsObjectElem *)m_pCurElem)->value = (jsBase*)str;
				}
				else {
					m_pCurElem = (jsBase *)str;
				}
				m_pCurCont->push_back(m_pCurElem);
			}
		}
		else if (a == eTOKEN_TO_INT_VALUE) {
			jsNumber *num = new jsNumber;
			num->num_value = last_int;
			if (m_iCurState == ePARSE_OBJVAL) {
				// curElem should be ObjKey
				((jsObjectElem *)m_pCurElem)->value = (jsBase*)num;
			}
			else {
				m_pCurElem = (jsBase *)num;
			}
			m_pCurCont->push_back(m_pCurElem);
		}
		else {
			// if (a == eTOKEN_TO_COMMA) {}
			// do nothing
		}
	}

	m_iCurState = s;
	return m_iCurState;
}

#define MAX_STRBUF	1023

static bool extract_string(istream &in, string &str)
{
	int c = 0;
	int i = 0;
	char ca[MAX_STRBUF+1];
	bool ret = false;

	str.clear();

	c = in.get();
	while (!in.eof()) {
		c = in.get();
		if (c == '\\') {
			// escape char ", \, /, b, f, n, r, t
			c = in.get(); // just consume
			if (c == '"') {
				ca[i++] = '"';
			}
			else if (c == '\\') {
				ca[i++] = '\\';
			}
			else if (c == '/') {
				ca[i++] = '/';
			}
			else if (c == 'b') {
				ca[i++] = '\b';
			}
			else if (c == 'f') {
				ca[i++] = '\f';
			}
			else if (c == 'n') {
				ca[i++] = '\n';
			}
			else if (c == 'r') {
				ca[i++] = '\r';
			}
			else if (c == 't') {
				ca[i++] = '\t';
			}
			else {
				// something wrong
				break;
			}
			if (i == MAX_STRBUF) {
				ca[i] = '\0';
				str.append(ca);
				i = 0;
			}
		}
		else if (c == '\"') {
			// closing
			ret = true;
			break;
		}
		else {
			ca[i++] = c;
			if (i == MAX_STRBUF) {
				ca[i] = '\0';
				str.append(ca);
				i = 0;
			}
		}
	}

	if (i > 0) {
		ca[i] = '\0';
		str.append(ca);
	}

	return ret;
}

static int extract_integer(istream &in)
{
	int i = 0;
	int result = 0;
	int c;
	bool ret = false;

	do {
		c = in.peek();
		if (!isdigit(c)) {
			break;
		}
		else {
			result = result * 10 + c - '0';
		}
		c = in.get();
	} while (!in.eof());

	return result;
}

static bool extract_reserved(istream &in, const char *reserved)
{
	bool ret = false;
	string str;

	in >> str;

	for (auto& c : str) {
		c = tolower(c);
	}

	if (str.compare(reserved)) {
		ret = true;
	}
	return ret;
}

eTOKEN_ACTION jsM::getToken(void)
{
	eTOKEN_ACTION ret = eTOKEN_TO_INVALID;
	bool b = false;
	int c = -1;

	last_str.clear();
	last_int = -1;

	while (!m_isIn.eof()) {
		c = m_isIn.peek();
		if (isspace(c)) {
			c = m_isIn.get();
		}
		else {
			break;
		}
	}

	switch (c) {
	case -1: // end of stream
		break;
	case '\"':
		b = extract_string(m_isIn, last_str);
		if (b) {
			ret = eTOKEN_TO_STR_VALUE;
		}
		break;
	case '-':
		c = m_isIn.get();
		last_int = extract_integer(m_isIn);
		if (last_int >= 0) {
			ret = eTOKEN_TO_INT_VALUE;
			last_int *= -1;
		}
		break;
	case '0': // FALL-THROUGH
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		last_int = extract_integer(m_isIn);
		if (last_int >= 0) {
			ret = eTOKEN_TO_INT_VALUE;
		}
		break;
	case '[':
		// push and array
		ret = eTOKEN_TO_ARRAY;
		c = m_isIn.get();
		break;
	case ']':
		ret = eTOKEN_TO_CLOSE;
		c = m_isIn.get();
		break;
	case '{':
		// push and object
		ret = eTOKEN_TO_OBJKEY;
		c = m_isIn.get();
		break;
	case '}':
		ret = eTOKEN_TO_CLOSE;
		c = m_isIn.get();
		break;
	case ':':
		c = m_isIn.get();
		if (m_iCurState == ePARSE_OBJVAL) {
			// skip to next token
			ret = getToken();
		}
		break;
	case ',':
		ret = eTOKEN_TO_COMMA;
		c = m_isIn.get();
		break;
	case 'f':
	case 'F':
		if (extract_reserved(m_isIn, "false")) {
			b = false;
			ret = eTOKEN_TO_INT_VALUE;
		}
		else {
			ret = eTOKEN_TO_INVALID;
		}
		break;
	case 't':
	case 'T':
		if (extract_reserved(m_isIn, "true")) {
			b = true;
			ret = eTOKEN_TO_INT_VALUE;
		}
		else {
			ret = eTOKEN_TO_INVALID;
		}
		break;
	case 'n':
	case 'N':
		if (extract_reserved(m_isIn, "null")) {
			ret = eTOKEN_TO_INT_VALUE;
		}
		else {
			ret = eTOKEN_TO_INVALID;
		}
		break;
	case '/':
		c = m_isIn.get();
		c = m_isIn.get();
		if (c == '/') {
			while (!m_isIn.eof()) {
				c = m_isIn.get();
				if (c == '\n' || c == '\r') {
					break;
				}
			}
			ret = getToken();
		}
		else if (c == '*') {
			while (!m_isIn.eof()) {
				c = m_isIn.get();
				if (c == '*') {
					c = m_isIn.get();
					if (c == '/') {
						break;
					}
				}
			}
			ret = getToken();
		}
		else {

		}
		break;
	default:
		cout << "### unknown " << c << endl;
		ret = eTOKEN_TO_INVALID;
		break;
	}

	return ret;
}

int main()
{
	string srcstr("{"\
					"\"RequestID\" : 1, 	\"Length\" : -1, \"Provider\" : \"SomeApp\",\"Condition\" : "\
						"["\
							"{"\
								"\"Field1\" : 0"\
							"}"\
						"], \"ConditionNotIn\" : "\
						"["\
							"{"\
								"\"Field2\" : 3"\
							"}"\
						"],// comments\n"\
						"\"Fields\" : "\
						"["\
							"{"\
								"\"name\" : \"Field1\", \"Type\" : \"UCHAR\", /*\"startBit\" : 0,*/ \"bitSize\" : 8, \"rangeFrom\" : 0, \"rangeTo\" : 10, \"validWhen\" : "
								"["\
									"0, 2, 4, 8, 10, \"Hello\""\
								"]"\
							"}"\
						"]"\
					"}");

	jsM test;

	test.m_isIn.str(srcstr);

	test.parse();

	test.print();

	return 0;
}


